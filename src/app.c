#include "win.h"
static HWND window;static HANDLE heap;
static void*mem(SIZE_T n){void*p=HeapAlloc(heap,8,n?n:1);if(!p){MessageBoxW(window,W("内存不足，程序将退出。"),W("序列组合工具"),16);ExitProcess(1);}return p;}
static void drop(void*p){if(p)HeapFree(heap,0,p);}
static void*resize(void*p,SIZE_T n){if(!p)return mem(n);void*q=HeapReAlloc(heap,8,p,n);if(!q){MessageBoxW(window,W("内存不足。"),W("错误"),16);ExitProcess(1);}return q;}
#include "core.h"
static unsigned wlen(const WCHAR*s){unsigned n=0;while(s[n])n++;return n;}
static WCHAR*wcat(WCHAR*d,const WCHAR*s){while(*s)*d++=*s++;*d=0;return d;}
static WCHAR*wide(const char*s){int n=MultiByteToWideChar(65001,8,s,-1,0,0);if(!n)return 0;WCHAR*w=mem((SIZE_T)n*2);MultiByteToWideChar(65001,8,s,-1,w,n);return w;}
static char*utf8(const WCHAR*w){int n=WideCharToMultiByte(65001,0,w,-1,0,0,0,0);char*s=mem(n);WideCharToMultiByte(65001,0,w,-1,s,n,0,0);return s;}
static WCHAR*getText(HWND h){int n=GetWindowTextLengthW(h)+1;WCHAR*s=mem((SIZE_T)n*2);GetWindowTextW(h,s,n);return s;}
static void setUTF(HWND h,const char*s){WCHAR*w=wide(s);if(w){SetWindowTextW(h,w);drop(w);}}
static void warn(const char*s){WCHAR*w=wide(s);MessageBoxW(window,w?w:W("输入有误。"),W("请检查"),48);drop(w);}

enum{ID_LIST=100,ID_NAME,ID_SEQ,ID_RC,ID_SKIP,ID_DEDUP,ID_AMBIG,ID_ADD,ID_REMOVE,ID_IMPORT,ID_COUNT,ID_TXT,ID_XLSX,ID_CANCEL,ID_HELP,ID_UP,ID_DOWN};
typedef struct{WCHAR name[80];WCHAR*input;int rc,skip;}Group;
static Group groups[32];static unsigned groupCount=2,current,nextGroupNumber=3;static int loading,busy,cancelled;
static HWND list,nameBox,seqBox,rcBox,skipBox,dedupBox,ambigBox,status,title,hint,buttons[10];
static HANDLE font,mono;static int ids[10]={ID_ADD,ID_REMOVE,ID_UP,ID_DOWN,ID_IMPORT,ID_COUNT,ID_TXT,ID_XLSX,ID_CANCEL,ID_HELP};
static const WCHAR*buttonLabels[]={W("＋ 添加组"),W("删除组"),W("上移"),W("下移"),W("导入 TXT / FASTA"),W("计算 / 预览"),W("TXT + XLSX"),W("导出 XLSX"),W("取消导出"),W("使用说明")};
static HWND control(const WCHAR*cls,const WCHAR*label,DWORD style,int id){HWND h=CreateWindowExW((style&0x00800000)?0x200:0,cls,label,0x50000000|style,0,0,100,30,window,(HANDLE)(SIZE_T)id,GetModuleHandleW(0),0);SendMessageW(h,0x30,(WPARAM)font,1);return h;}
static int checked(HWND h){return (int)SendMessageW(h,0xF0,0,0)==1;}
static void saveCurrent(void){if(loading||current>=groupCount)return;GetWindowTextW(nameBox,groups[current].name,80);drop(groups[current].input);groups[current].input=getText(seqBox);groups[current].rc=checked(rcBox);groups[current].skip=checked(skipBox);}
static void refreshList(void){loading=1;SendMessageW(list,0x184,0,0);for(unsigned i=0;i<groupCount;i++){WCHAR label[110];char num[24];number(num,i+1);WCHAR*n=wide(num);WCHAR*p=wcat(label,n);drop(n);p=wcat(p,W(". "));wcat(p,groups[i].name);SendMessageW(list,0x180,0,(LPARAM)label);}SendMessageW(list,0x186,current,0);loading=0;}
static void loadCurrent(void){loading=1;SetWindowTextW(nameBox,groups[current].name);SetWindowTextW(seqBox,groups[current].input?groups[current].input:W(""));SendMessageW(rcBox,0xF1,groups[current].rc,0);SendMessageW(skipBox,0xF1,groups[current].skip,0);loading=0;}
static void layout(void){RECT r;GetClientRect(window,&r);int width=r.right,height=r.bottom;if(width<760)width=760;if(height<620)height=620;
 MoveWindow(title,22,14,width-44,35,1);MoveWindow(hint,22,52,width-44,44,1);MoveWindow(list,22,108,195,height-286,1);
 MoveWindow(buttons[0],22,height-166,94,30,1);MoveWindow(buttons[1],123,height-166,94,30,1);MoveWindow(buttons[2],22,height-130,94,30,1);MoveWindow(buttons[3],123,height-130,94,30,1);
 MoveWindow(nameBox,239,108,width-263,30,1);MoveWindow(seqBox,239,148,width-263,height-396,1);
 MoveWindow(rcBox,239,height-238,width-263,28,1);MoveWindow(skipBox,239,height-208,215,28,1);MoveWindow(buttons[4],width-231,height-204,207,30,1);
 MoveWindow(dedupBox,239,height-174,230,28,1);MoveWindow(ambigBox,480,height-174,width-504,28,1);
 MoveWindow(buttons[5],239,height-134,132,34,1);MoveWindow(buttons[6],383,height-134,132,34,1);MoveWindow(buttons[7],527,height-134,132,34,1);MoveWindow(buttons[8],671,height-134,132,34,1);
 MoveWindow(buttons[9],22,height-90,195,30,1);MoveWindow(status,239,height-90,width-263,69,1);
}
static int sameWide(const WCHAR*a,const WCHAR*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void groupAction(int id){saveCurrent();if(id==ID_ADD){if(groupCount==32){warn("最多支持 32 组序列。");return;}current=groupCount++;
  int unique=0;while(!unique){char t[24];number(t,nextGroupNumber++);WCHAR*w=wide(t);wcat(wcat(groups[current].name,W("Group")),w);drop(w);unique=1;
   for(unsigned i=0;i<current;i++)if(sameWide(groups[current].name,groups[i].name))unique=0;}
  groups[current].input=mem(2);}
 else if(id==ID_REMOVE){if(groupCount<=2){warn("至少需要保留两组序列。");return;}drop(groups[current].input);for(unsigned i=current;i+1<groupCount;i++)groups[i]=groups[i+1];groupCount--;groups[groupCount]=(Group){0};if(current>=groupCount)current=groupCount-1;}
 else if(id==ID_UP&&current){Group g=groups[current];groups[current]=groups[current-1];groups[--current]=g;}
 else if(id==ID_DOWN&&current+1<groupCount){Group g=groups[current];groups[current]=groups[current+1];groups[++current]=g;}
 refreshList();loadCurrent();SetWindowTextW(status,W("组顺序决定结果列顺序。请点击“计算 / 预览”刷新组合数量。"));}
static int chooseFile(WCHAR*path,int save,int xlsx){OPENFILENAMEW f={0};f.size=sizeof(f);f.owner=window;f.file=path;f.maxFile=32760;f.filterIndex=1;f.flags=0x80000|0x800|8|(save?2:0x1000);f.defExt=save?(xlsx?W("xlsx"):W("txt")):0;
 f.filter=save?(xlsx?W("Excel 工作簿 (*.xlsx)\0*.xlsx\0所有文件\0*.*\0"):W("制表符文本 (*.txt)\0*.txt\0所有文件\0*.*\0")):W("序列文本 (*.txt;*.tsv;*.fa;*.fasta)\0*.txt;*.tsv;*.fa;*.fasta\0所有文件\0*.*\0");
 return save?GetSaveFileNameW(&f):GetOpenFileNameW(&f);}
static void importText(void){WCHAR*path=mem(65536);if(!chooseFile(path,0,0)){drop(path);return;}
 HANDLE h=CreateFileW(path,0x80000000,1,0,3,0,0);drop(path);if(h==(HANDLE)-1){warn("文件无法读取。请检查文件位置和访问权限。");return;}
 long long size=0;if(!GetFileSizeEx(h,&size)||size<0||size>16*1024*1024){CloseHandle(h);warn("单组输入文件最大支持 16 MB。");return;}
 char*data=mem((SIZE_T)size+4);DWORD got=0;BOOL ok=ReadFile(h,data,(DWORD)size,&got,0);CloseHandle(h);if(!ok||got!=(DWORD)size){drop(data);warn("文件读取不完整。");return;}
 /* Reject binary/embedded-NUL input before any zero-terminated conversion. */
 if(size>1&&data[0]=='P'&&data[1]=='K'){drop(data);warn("这是 XLSX 压缩文件。请在 Excel 中复制碱基序列这一列，粘贴到输入框；或另存为 TXT 后导入。");return;}
 int utf16=size>=2&&(((unsigned char)data[0]==0xFF&&(unsigned char)data[1]==0xFE)||((unsigned char)data[0]==0xFE&&(unsigned char)data[1]==0xFF));
 if(utf16&&(size%2)){drop(data);warn("UTF-16 文件字节数不完整，可能已损坏；未导入任何内容。");return;}
 if(!utf16)for(SIZE_T i=0;i<(SIZE_T)size;i++)if(!data[i]){drop(data);warn("文件含空字符或不是受支持的文本编码；未导入任何内容。");return;}
 WCHAR*w=0;
 if(size>=2&&(unsigned char)data[0]==0xFF&&(unsigned char)data[1]==0xFE){w=mem((SIZE_T)size+2);bytes(w,data+2,(SIZE_T)size-2);}
 else if(size>=2&&(unsigned char)data[0]==0xFE&&(unsigned char)data[1]==0xFF){w=mem((SIZE_T)size+2);for(unsigned i=2;i+1<(unsigned)size;i+=2)w[(i-2)/2]=((unsigned char)data[i]<<8)|(unsigned char)data[i+1];}
 else {const char*s=data;if(size>=3&&(unsigned char)data[0]==0xEF&&(unsigned char)data[1]==0xBB&&(unsigned char)data[2]==0xBF)s+=3;w=wide(s);}
 if(!w){drop(data);warn("文本编码无法识别。请将文件另存为 UTF-8 或 UTF-16 文本。");return;}
 if(utf16){int invalid=0;unsigned units=(unsigned)(size-2)/2;
  for(unsigned i=0;i<units;i++){unsigned c=w[i];if(!c){invalid=1;break;}
   if(c>=0xD800&&c<=0xDBFF){if(i+1>=units||w[i+1]<0xDC00||w[i+1]>0xDFFF){invalid=1;break;}i++;}
   else if(c>=0xDC00&&c<=0xDFFF){invalid=1;break;}}
  if(invalid){drop(data);drop(w);warn("UTF-16 文件包含空字符或无效字符编码；未导入任何内容。");return;}}
 drop(data);SetWindowTextW(seqBox,w);drop(w);SendMessageW(skipBox,0xF1,0,0);saveCurrent();SetWindowTextW(status,W("已导入当前组。若第一行是列名，请勾选“首个非空行为表头”。"));}
static void freeAll(Parsed*p){for(unsigned i=0;i<groupCount;i++)releaseParsed(&p[i]);}
static int validNames(void){
 WCHAR headers[64][84];unsigned count=0;
 for(unsigned i=0;i<groupCount;i++){
  WCHAR*name=groups[i].name;unsigned start=0,n=wlen(name);
  while(start<n&&name[start]==' ')start++;
  while(n>start&&name[n-1]==' ')n--;
  unsigned size=n-start;for(unsigned j=0;j<size;j++)name[j]=name[start+j];name[size]=0;
  if(!size){warn("每组都需要填写名称，不能仅包含空格。");return 0;}
  for(unsigned j=0;j<size;j++)if(name[j]<32||name[j]==127||name[j]==0xFFFE||name[j]==0xFFFF){warn("组名不能包含制表符、换行符或其他控制字符。");return 0;}
  for(int rc=0;rc<=(groups[i].rc?1:0);rc++){
   wcat(headers[count],name);if(rc)wcat(headers[count]+size,W("_RC"));
   for(unsigned j=0;j<count;j++)if(sameWide(headers[count],headers[j])){warn("输出列名重复。请修改组名，避免与其他组名或其 _RC 列重名。");return 0;}
   count++;
  }
 }
 return 1;
}
static int prepare(Parsed*p,U64*total){saveCurrent();refreshList();int dedup=checked(dedupBox),ambig=checked(ambigBox);
 if(!validNames())return 0;
 refreshList();loadCurrent();
 for(unsigned i=0;i<groupCount;i++){
  char*s=utf8(groups[i].input?groups[i].input:W(""));p[i]=parseSequences(s,groups[i].skip,dedup,ambig);drop(s);
  if(p[i].error){char msg[640],*b=textcat(msg,"第 ");b=number(b,i+1);b=textcat(b," 组：");
   if(p[i].error==2)b=textcat(b,"没有可用序列。");else{b=textcat(b,"第 ");b=number(b,p[i].errorLine);b=textcat(b,p[i].error==3?" 行附近存在空的 FASTA 记录。":p[i].error==4?" 行含多个由制表符分隔的字段。请每次只粘贴一列序列，程序不会将多列拼接。":" 行含不支持的字符。请只粘贴一列碱基序列，并检查表头、数字、U、短横线或氨基酸序列。");}warn(msg);return 0;
  }
 }
 if(!combinationCount(p,groupCount,total)){warn("组合数超出可表示的范围，请减少组数或序列数量。");return 0;}return 1;}
static void preview(void){Parsed p[32]={0};U64 total;if(!prepare(p,&total)){freeAll(p);return;}char msg[8192],*b=textcat(msg,"组合数：");b=number(b,total);b=textcat(b,"\r\n各组有效序列数：");for(unsigned i=0;i<groupCount;i++){if(i)b=textcat(b," × ");b=number(b,p[i].n);}unsigned removed=0;for(unsigned i=0;i<groupCount;i++)removed+=p[i].dupes;b=textcat(b,"；去重移除：");b=number(b,removed);b=textcat(b," 条。");setUTF(status,msg);freeAll(p);}
static HANDLE outputHandle;static int ioOK;static void output(const void*p,unsigned n){if(!ioOK)return;DWORD wrote=0;if(!WriteFile(outputHandle,p,n,&wrote,0)||wrote!=n)ioOK=0;}
#include "zip.h"
static void setBusy(int value){busy=value;EnableWindow(list,!value);EnableWindow(nameBox,!value);EnableWindow(seqBox,!value);EnableWindow(rcBox,!value);EnableWindow(skipBox,!value);EnableWindow(dedupBox,!value);EnableWindow(ambigBox,!value);for(int i=0;i<10;i++)EnableWindow(buttons[i],i==8?value:!value);}
static void pump(U64 done,U64 total){char msg[180],*b=textcat(msg,"正在导出：");b=number(b,done);b=textcat(b," / ");b=number(b,total);setUTF(status,msg);MSG m;while(PeekMessageW(&m,0,0,0,1)){TranslateMessage(&m);DispatchMessageW(&m);}}
#include "exporter.h"
static void help(void){MessageBoxW(window,W("1. 程序初始提供 Group1、Group2。根据需要添加或删除组（2–32 组），组名均可修改。\n2. 每组粘贴一列碱基序列，每行一条；或导入 TXT/FASTA。\n   XLSX 请在 Excel 中复制碱基序列列，或另存为 TXT 后导入。\n3. 有表头时勾选“首个非空行为表头”。FASTA 自动识别并拼接换行。\n4. 需要某组的反向互补序列时，在该组中勾选“增加本组反向互补列”。每组独立设置，初始均不勾选。\n5. 点击“计算 / 预览”，确认后导出 TXT 或 XLSX。\n\n组合为笛卡尔积：每组各选一条，左侧第一组变化最慢。\n组内去重默认开启；保留原顺序，不合并不同组的相同序列。\n支持大小写；移除序列中的空格，含多列制表符的输入会被拒绝。\n勾选简并碱基时支持 IUPAC：R Y S W K M B D H V N。\n简并符号保留原样，不展开为 A/C/G/T 的额外组合。\n反向互补按 DNA 5′→3′ 输出，不含 RNA 的 U。\n\nTXT 使用 UTF-16LE 编码、制表符分列，第一行为列名。选择 TXT + XLSX 会同时生成配套 Excel 工作簿；直接打开 XLSX 即可查看分列结果。\nXLSX 单表上限 1,048,575 种组合，超过时可导出 TXT。\n程序在本机离线处理，不上传序列。"),W("序列组合工具 1.2.1 · 使用说明"),64);}
static LRESULT procedure(HWND h,UINT message,WPARAM wp,LPARAM lp){
 if(message==1){window=h;font=CreateFontW(-17,0,0,0,400,0,0,0,1,0,0,5,0,W("Microsoft YaHei UI"));mono=CreateFontW(-17,0,0,0,400,0,0,0,1,0,0,5,0,W("Consolas"));
  title=control(W("STATIC"),W("碱基序列组合工具"),0,0);hint=control(W("STATIC"),W("每组各取一条，生成全部组合。原序列与可选的反向互补序列分别成列。\r\n将 Excel 的一列序列粘贴到右侧；也可导入 TXT / FASTA。"),0,0);
  list=control(W("LISTBOX"),W(""),0x00800000|0x00200000|0x00010000|1,ID_LIST);
  nameBox=control(W("EDIT"),W(""),0x00800000|0x00010000|0x80,ID_NAME);SendMessageW(nameBox,0xC5,70,0);
  seqBox=control(W("EDIT"),W(""),0x00800000|0x00300000|0x00010000|4|0x40|0x80|0x1000,ID_SEQ);SendMessageW(seqBox,0xC5,16*1024*1024,0);SendMessageW(seqBox,0x30,(WPARAM)mono,1);
  rcBox=control(W("BUTTON"),W("增加本组反向互补列（原序列列仍保留）"),0x10003,ID_RC);skipBox=control(W("BUTTON"),W("首个非空行为表头"),0x10003,ID_SKIP);
  dedupBox=control(W("BUTTON"),W("组内去重（保留首次出现）"),0x10003,ID_DEDUP);SendMessageW(dedupBox,0xF1,1,0);
  ambigBox=control(W("BUTTON"),W("允许 IUPAC 简并碱基"),0x10003,ID_AMBIG);
  for(int i=0;i<10;i++)buttons[i]=control(W("BUTTON"),buttonLabels[i],0x10000,ids[i]);
  EnableWindow(buttons[8],0);
  status=control(W("EDIT"),W("填写至少两组序列，然后点击“计算 / 预览”。"),4|0x800,0);
  wcat(groups[0].name,W("Group1"));wcat(groups[1].name,W("Group2"));groups[0].input=mem(2);groups[1].input=mem(2);refreshList();loadCurrent();layout();return 0;
 }
 if(message==5){if(list)layout();return 0;}
 if(message==0x24){/* MINMAXINFO: minimum window size at index 6/7. */int*p=(int*)lp;p[6]=900;p[7]=680;return 0;}
 if(message==0x111){int id=(int)(wp&0xFFFF),code=(int)((wp>>16)&0xFFFF);if(busy){if(id==ID_CANCEL)cancelled=1;return 0;}
  if(id==ID_LIST&&code==1&&!loading){saveCurrent();LRESULT n=SendMessageW(list,0x188,0,0);if(n>=0&&(unsigned)n<groupCount)current=(unsigned)n;refreshList();loadCurrent();return 0;}
  if(code==0){if(id==ID_ADD||id==ID_REMOVE||id==ID_UP||id==ID_DOWN)groupAction(id);else if(id==ID_IMPORT)importText();else if(id==ID_COUNT)preview();else if(id==ID_TXT)exportFile(0);else if(id==ID_XLSX)exportFile(1);else if(id==ID_HELP)help();}return 0;
 }
 if(message==0x10){if(busy){cancelled=1;return 0;}DestroyWindow(h);return 0;}
 if(message==2){DeleteObject(font);DeleteObject(mono);PostQuitMessage(0);return 0;}return DefWindowProcW(h,message,wp,lp);
}
void entry(void){heap=GetProcessHeap();SetProcessDPIAware();WNDCLASSW wc={0};wc.proc=procedure;wc.instance=GetModuleHandleW(0);wc.cursor=LoadCursorW(0,(const WCHAR*)32512);wc.background=(HANDLE)16;wc.name=W("SequenceCombinerWindow");
 if(!RegisterClassW(&wc))ExitProcess(1);
 HWND h=CreateWindowExW(0,wc.name,W("序列组合工具 1.2.1 · 离线版"),0x00CF0000,80,60,1040,760,0,0,wc.instance,0);
 if(!h)ExitProcess(1);
 ShowWindow(h,5);UpdateWindow(h);MSG msg;while(GetMessageW(&msg,0,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}ExitProcess(0);}
