/* Runs the actual PE machine code against Win32 API test doubles on Linux.
   This checks ABI/relocation and export execution; it is not a Windows UI test. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <setjmp.h>
#include <assert.h>
#include <signal.h>
#include <ucontext.h>
#include <errno.h>
#define A __attribute__((ms_abi))
typedef uint16_t W;typedef void*H;
typedef struct{int id,check,selected;W*text;}Control;
typedef intptr_t(A*Proc)(H,unsigned,uintptr_t,intptr_t);
typedef struct{unsigned style;Proc proc;int a,b;H instance,icon,cursor,bg;const W*menu,*name;}WC;
typedef struct{unsigned size;H owner,instance;const W*filter;W*custom;unsigned maxcustom,index;W*file;unsigned maxfile;W*title;unsigned maxtitle;const W*dir,*caption;unsigned flags;uint16_t off,ext;const W*defext;intptr_t data;void*hook;const W*templ;void*reserved;unsigned word,ex;}OFN;
static Proc proc;static Control*ctl[130];static H root;static jmp_buf done;static const char*arg1,*arg2,*mode="full";static int saved,warnings;
static uint64_t written;static int failWrite,cancelExport;static const char*importName;static const char*saveName;
static W*from8(const char*s){W*w=calloc(strlen(s)+2,2);unsigned n=0;
 while(*s){unsigned char c=*s++;unsigned v=c,extra=0,min=0;
  if(c>=0xC2&&c<=0xDF){v=c&31;extra=1;min=128;}else if(c>=0xE0&&c<=0xEF){v=c&15;extra=2;min=2048;}else if(c>=0xF0&&c<=0xF4){v=c&7;extra=3;min=65536;}else if(c>=128)goto bad;
  for(unsigned i=0;i<extra;i++){unsigned d=(unsigned char)*s;if(d<128||d>191)goto bad;s++;v=(v<<6)|(d&63);}
  if(v<min||v>0x10FFFF||(v>=0xD800&&v<=0xDFFF))goto bad;
  if(v>=65536){v-=65536;w[n++]=0xD800+(v>>10);w[n++]=0xDC00+(v&1023);}else w[n++]=v;
 }return w;
bad: free(w);return 0;}
static char*to8(const W*w){unsigned units=0;while(w&&w[units])units++;char*s=calloc(units*3+1,1);unsigned n=0;
 while(w&&*w){unsigned c=*w++;if(c>=0xD800&&c<=0xDBFF&&*w>=0xDC00&&*w<=0xDFFF){c=65536+((c-0xD800)<<10)+(*w++-0xDC00);}
  else if(c>=0xD800&&c<=0xDFFF)c=0xFFFD;
  if(c<128)s[n++]=c;else if(c<2048){s[n++]=192|(c>>6);s[n++]=128|(c&63);}else if(c<65536){s[n++]=224|(c>>12);s[n++]=128|((c>>6)&63);s[n++]=128|(c&63);}else{s[n++]=240|(c>>18);s[n++]=128|((c>>12)&63);s[n++]=128|((c>>6)&63);s[n++]=128|(c&63);}}
 return s;}
static unsigned len(const W*w){unsigned n=0;while(w&&w[n])n++;return n;}
static W*dupw(const W*w){unsigned n=len(w);W*q=calloc(n+1,2);if(w)memcpy(q,w,n*2);return q;}
H A GetModuleHandleW(const W*x){return(H)1;}H A GetProcessHeap(void){return(H)1;}
void*A HeapAlloc(H h,unsigned f,uint64_t n){return calloc(1,n);}void*A HeapReAlloc(H h,unsigned f,void*p,uint64_t n){return realloc(p,n);}int A HeapFree(H h,unsigned f,void*p){free(p);return 1;}
void A ExitProcess(unsigned code){assert(code==0);longjmp(done,1);}
unsigned short A RegisterClassW(const WC*w){proc=w->proc;return 1;}
H A CreateWindowExW(unsigned ex,const W*cl,const W*t,unsigned st,int x,int y,int w,int h,H parent,H id,H instance,void*p){Control*c=calloc(1,sizeof(*c));c->text=dupw(t);c->id=(int)(intptr_t)id;if(!parent){root=c;proc(c,1,0,0);}else if(c->id>0&&c->id<130)ctl[c->id]=c;return c;}
intptr_t A DefWindowProcW(H h,unsigned m,uintptr_t w,intptr_t l){return 0;}
int A ShowWindow(H h,int s){return 1;}int A UpdateWindow(H h){return 1;}
int A TranslateMessage(void*m){return 1;}intptr_t A DispatchMessageW(void*m){return 0;}void A PostQuitMessage(int n){}
int A SetWindowTextW(H h,const W*w){Control*c=h;free(c->text);c->text=dupw(w);return 1;}
int A GetWindowTextW(H h,W*w,int max){Control*c=h;unsigned n=len(c->text);if(n>=(unsigned)max)n=max-1;memcpy(w,c->text,n*2);w[n]=0;return n;}
int A GetWindowTextLengthW(H h){return len(((Control*)h)->text);}
intptr_t A SendMessageW(H h,unsigned m,uintptr_t w,intptr_t l){Control*c=h;if(m==0xF0)return c->check;if(m==0xF1)c->check=w;if(m==0x186)c->selected=w;if(m==0x188)return c->selected;return 0;}
int A MessageBoxW(H h,const W*w,const W*t,unsigned flags){char*s=to8(w);fprintf(stderr,"MESSAGE: %s\n",s);free(s);assert(flags!=16);if(flags==48)warnings++;return 6;}
H A LoadCursorW(H h,const W*w){return(H)1;}int A MoveWindow(H h,int a,int b,int c,int d,int e){return 1;}int A GetClientRect(H h,int*r){r[0]=r[1]=0;r[2]=1024;r[3]=720;return 1;}int A EnableWindow(H h,int e){return 1;}int A PeekMessageW(void*p,H h,unsigned a,unsigned b,unsigned c){if(cancelExport){proc(root,0x111,113,0);cancelExport=0;}return 0;}int A SetProcessDPIAware(void){return 1;}int A DestroyWindow(H h){return 1;}
H A CreateFontW(int a,int b,int c,int d,int e,unsigned f,unsigned g,unsigned h,unsigned i,unsigned j,unsigned k,unsigned l,unsigned m,const W*n){return(H)1;}int A DeleteObject(H h){return 1;}
int A MultiByteToWideChar(unsigned cp,unsigned flags,const char*s,int n,W*out,int max){assert(cp==65001&&n==-1);W*w=from8(s);if(!w)return 0;int count=len(w)+1;if(out){assert(max>=count);memcpy(out,w,count*2);}free(w);return count;}
int A WideCharToMultiByte(unsigned cp,unsigned flags,const W*w,int n,char*out,int max,const char*d,int*used){assert(cp==65001&&n==-1);char*s=to8(w);int count=strlen(s)+1;if(out){assert(max>=count);memcpy(out,s,count);}free(s);return count;}
H A CreateFileW(const W*w,unsigned access,unsigned share,void*sec,unsigned disp,unsigned attrs,H temp){char*p=to8(w);if(!strcmp(mode,"companion_failure")&&strstr(p,".xlsx.partial"))failWrite=1;int fd=open(p,disp==1?O_WRONLY|O_CREAT|O_EXCL:O_RDONLY,0600);free(p);return fd<0?(H)-1:(H)(intptr_t)(fd+1);}
int A ReadFile(H h,void*p,unsigned n,unsigned*got,void*o){ssize_t r=read((int)(intptr_t)h-1,p,n);*got=r<0?0:r;return r>=0;}
int A WriteFile(H h,const void*p,unsigned n,unsigned*got,void*o){if(failWrite&&written>4000){*got=0;return 0;}ssize_t r=write((int)(intptr_t)h-1,p,n);*got=r<0?0:r;if(r>0)written+=r;return r>=0;}
int A CloseHandle(H h){return close((int)(intptr_t)h-1)==0;}int A GetFileSizeEx(H h,long long*n){struct stat s;if(fstat((int)(intptr_t)h-1,&s))return 0;*n=s.st_size;return 1;}
int A DeleteFileW(const W*w){char*p=to8(w);int r=unlink(p);free(p);return r==0;}int A MoveFileExW(const W*a,const W*b,unsigned flags){char*x=to8(a),*y=to8(b);int r;if(flags&1)r=rename(x,y);else{r=link(x,y);if(!r)unlink(x);}free(x);free(y);return r==0;}int A FlushFileBuffers(H h){return fsync((int)(intptr_t)h-1)==0;}unsigned A GetLastError(void){return 0;}
static int fillPath(OFN*f,const char*p){W*w=from8(p);assert(w);memcpy(f->file,w,(len(w)+1)*2);free(w);return 1;}
int A GetOpenFileNameW(OFN*f){return importName?fillPath(f,importName):0;}
int A GetSaveFileNameW(OFN*f){const char*p=saveName?saveName:(saved?"native_result.xlsx":"native_result.txt");saved++;return fillPath(f,p);}
unsigned A GetFileAttributesW(const W*w){char*p=to8(w);struct stat s;int result=stat(p,&s);free(p);return result?0xFFFFFFFFU:0x80;}
static void loadSeq(const char*file){FILE*f=fopen(file,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);char*s=calloc(n+1,1);assert(fread(s,1,n,f)==(size_t)n);fclose(f);W*w=from8(s);SetWindowTextW(ctl[102],w);free(w);free(s);}
static void set(int id,const char*s){W*w=from8(s);assert(w);SetWindowTextW(ctl[id],w);free(w);}
static void expect(int id,const char*s){char*t=to8(ctl[id]->text);assert(!strcmp(t,s));free(t);}
static void selectGroup(int i){ctl[100]->selected=i;proc(root,0x111,100|(1<<16),0);}
static void command(int id){proc(root,0x111,id,0);}
int A GetMessageW(void*m,H h,unsigned a,unsigned b){
 if(!strcmp(mode,"names")){
  command(107);expect(101,"Group3");assert(!ctl[103]->check);selectGroup(1);command(108);
  command(107);expect(101,"Group4");assert(!ctl[103]->check);set(101,"Group5");command(107);expect(101,"Group6");return 0;
 }
 if(!strncmp(mode,"import_",7)){
  set(102,"AC");importName=arg1;command(109);
  if(!strcmp(mode,"import_bad")){assert(warnings==1);expect(102,"AC");}
  else{assert(!warnings);char*actual=to8(ctl[102]->text);loadSeq(arg2);char*expected=to8(ctl[102]->text);assert(!strcmp(actual,expected));free(actual);free(expected);}
  return 0;
 }
 if(!strcmp(mode,"validation")){
  set(102,"AA");selectGroup(1);set(102,"CC\tGG");command(111);assert(warnings==1&&!saved);
  set(102,"CC");set(101,"Bad\tName");command(111);assert(warnings==2&&!saved);
  set(101,"   ");command(111);assert(warnings==3&&!saved);
  set(101,"Group1_RC");selectGroup(0);ctl[103]->check=1;command(111);assert(warnings==4&&!saved);
  selectGroup(1);set(101,"Group1");command(111);assert(warnings==5&&!saved);return 0;
 }
 if(!strcmp(mode,"columns64")){
  for(int i=0;i<32;i++){if(i>=2)command(107);else selectGroup(i);set(102,"AC");ctl[103]->check=1;}
  saveName="columns64.xlsx";command(112);assert(saved==1&&!warnings);return 0;
 }
 if(strcmp(mode,"full")){
  loadSeq(arg1);selectGroup(1);loadSeq(arg2);ctl[103]->check=1;
  if(!strcmp(mode,"extension"))saveName="wrong.xlsx";
  if(!strcmp(mode,"failure"))failWrite=1;
  if(!strcmp(mode,"cancel"))cancelExport=1;
  if(!strcmp(mode,"unicode")){set(101,"组二😀");selectGroup(0);set(101," A&_x0041_<\" ");ctl[103]->check=1;}
  command(111);
  if(!strcmp(mode,"failure")||!strcmp(mode,"stale")||!strcmp(mode,"extension"))assert(warnings==1);
  else assert(!warnings);
  assert(saved==1);return 0;
 }
 char*n=to8(ctl[101]->text);assert(!strcmp(n,"Group1"));free(n);assert(ctl[103]->check==0);
 proc(root,0x111,107,0);n=to8(ctl[101]->text);assert(!strcmp(n,"Group3"));free(n);assert(ctl[103]->check==0);proc(root,0x111,108,0);
 ctl[100]->selected=0;proc(root,0x111,100|(1<<16),0);loadSeq(arg1);ctl[100]->selected=1;proc(root,0x111,100|(1<<16),0);
 n=to8(ctl[101]->text);assert(!strcmp(n,"Group2"));free(n);assert(ctl[103]->check==0);
 loadSeq(arg2);ctl[103]->check=1;proc(root,0x111,110,0);proc(root,0x111,111,0);proc(root,0x111,112,0);assert(saved==2&&!warnings);return 0;}
static uint16_t r16(void*p){uint16_t v;memcpy(&v,p,2);return v;}static uint32_t r32(void*p){uint32_t v;memcpy(&v,p,4);return v;}static uint64_t r64(void*p){uint64_t v;memcpy(&v,p,8);return v;}
static void crash(int sig,siginfo_t*info,void*context){ucontext_t*c=context;fprintf(stderr,"fault address=%p rip=%llx rsp=%llx\n",info->si_addr,(unsigned long long)c->uc_mcontext.gregs[REG_RIP],(unsigned long long)c->uc_mcontext.gregs[REG_RSP]);_exit(99);}
int main(int argc,char**argv){struct sigaction sa={.sa_sigaction=crash,.sa_flags=SA_SIGINFO};sigaction(SIGSEGV,&sa,0);assert(argc==4||argc==5);arg1=argv[2];arg2=argv[3];if(argc==5)mode=argv[4];FILE*f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);unsigned char*file=malloc(n);assert(fread(file,1,n,f)==(size_t)n);fclose(f);unsigned pe=r32(file+0x3C);unsigned char*opt=file+pe+24;assert(r16(opt)==0x20B);uint64_t base=r64(opt+24);uint32_t size=r32(opt+56);unsigned char*image=mmap((void*)base,size,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);assert(image==(void*)base);memcpy(image,file,r32(opt+60));unsigned sections=r16(file+pe+6),optionalSize=r16(file+pe+20);unsigned char*sh=file+pe+24+optionalSize;
 for(unsigned i=0;i<sections;i++,sh+=40){unsigned raw=r32(sh+16);if(raw)memcpy(image+r32(sh+12),file+r32(sh+20),raw);}
 unsigned char*desc=image+r32(opt+112+8);while(r32(desc+12)){uint64_t*names=(void*)(image+r32(desc));uint64_t*iat=(void*)(image+r32(desc+16));for(unsigned i=0;names[i];i++){assert(!(names[i]>>63));char*name=(void*)(image+names[i]+2);void*fn=dlsym(RTLD_DEFAULT,name);if(!fn){fprintf(stderr,"Missing %s\n",name);abort();}iat[i]=(uintptr_t)fn;}desc+=20;}
 void(A*entry)(void)=(void*)(image+r32(opt+16));if(!setjmp(done))entry();puts("Actual PE code completed with Win32 test doubles.");return 0;}
