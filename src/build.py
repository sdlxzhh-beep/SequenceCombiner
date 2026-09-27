from pathlib import Path
import subprocess
from elf_to_coff import convert
root=Path(__file__).resolve().parent.parent
b=root/'build'; b.mkdir(exist_ok=True)
flags=['-m64','-mabi=ms','-mcmodel=large','-fstack-clash-protection','-fshort-wchar','-ffreestanding','-fno-builtin','-fno-stack-protector','-fno-asynchronous-unwind-tables','-fno-unwind-tables','-mno-red-zone','-fno-pic','-O2']
libs={
 'kernel32':['GetModuleHandleW','GetProcessHeap','HeapAlloc','HeapReAlloc','HeapFree','ExitProcess','CreateFileW','ReadFile','WriteFile','CloseHandle','GetFileSizeEx','DeleteFileW','MoveFileExW','FlushFileBuffers','MultiByteToWideChar','WideCharToMultiByte','GetLastError','GetFileAttributesW'],
 'user32':['RegisterClassW','CreateWindowExW','DefWindowProcW','ShowWindow','UpdateWindow','GetMessageW','TranslateMessage','DispatchMessageW','PostQuitMessage','SendMessageW','SetWindowTextW','GetWindowTextW','GetWindowTextLengthW','MessageBoxW','LoadCursorW','MoveWindow','GetClientRect','EnableWindow','PeekMessageW','SetProcessDPIAware','DestroyWindow'],
 'gdi32':['CreateFontW','DeleteObject'],
 'comdlg32':['GetOpenFileNameW','GetSaveFileNameW'],
}
def run(args): subprocess.run(args,check=True,cwd=b)
for lib,names in libs.items():
 (b/(lib+'.c')).write_text('\n'.join('void '+n+'(void) {}' for n in names))
 (b/(lib+'.def')).write_text('LIBRARY '+lib+'.dll\nEXPORTS\n'+'\n'.join(names)+'\n')
 run(['gcc',*flags,'-c',lib+'.c','-o',lib+'.elf'])
 convert(b/(lib+'.elf'),b/(lib+'.o'))
 run(['ld','-mi386pep','--dll','--entry','0',lib+'.o',lib+'.def','--out-implib',lib+'.a','-o',lib+'.dll'])
run(['gcc',*flags,'-Wall','-Wextra','-Wno-unused-parameter','-c',str(root/'src/app.c'),'-o','app.elf'])
convert(b/'app.elf',b/'app.o')
run(['ld','-mi386pep','--subsystem','windows','--entry','entry','--image-base','0x140000000','--dynamicbase','--nxcompat','--stack','8388608,1048576','app.o',*[lib+'.a' for lib in libs],'-o',str(root/'SequenceCombiner.exe')])
print(root/'SequenceCombiner.exe')
