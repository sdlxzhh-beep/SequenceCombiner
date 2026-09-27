/* SequenceCombiner 1.2.1. Portable core. Memory functions supplied by caller. */
typedef unsigned long long U64;
static unsigned slen(const char *s){unsigned n=0;while(s[n])n++;return n;}
static int same(const char*a,const char*b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static void bytes(void*d,const void*s,U64 n){unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;}
static char *copy(const char*s){unsigned n=slen(s)+1;char*p=mem(n);bytes(p,s,n);return p;}
static char comp(char c){switch(c){case'A':return'T';case'T':return'A';case'C':return'G';case'G':return'C';case'R':return'Y';case'Y':return'R';case'S':return'S';case'W':return'W';case'K':return'M';case'M':return'K';case'B':return'V';case'V':return'B';case'D':return'H';case'H':return'D';case'N':return'N';default:return 0;}}
static void reverseComplement(const char*s,char*out){unsigned n=slen(s);for(unsigned i=0;i<n;i++)out[i]=comp(s[n-i-1]);out[n]=0;}
typedef struct {char **seq;unsigned n,raw,dupes;unsigned errorLine;int error;} Parsed;
static U64 hash(const char*s){U64 h=1469598103934665603ULL;while(*s){h^=(unsigned char)*s++;h*=1099511628211ULL;}return h;}
static void releaseParsed(Parsed*p){for(unsigned i=0;i<p->n;i++)drop(p->seq[i]);drop(p->seq);*p=(Parsed){0};}
static void appendRecord(Parsed*p,char*buf,unsigned len,unsigned *cap){
 if(!len)return;
 buf[len]=0;
 if(p->n==*cap){*cap=*cap?*cap*2:64;p->seq=resize(p->seq,(U64)*cap*sizeof(char*));}
 p->seq[p->n++]=copy(buf);p->raw++;
}
/* Plain text is one sequence per nonblank line. FASTA combines wrapped lines. */
static Parsed parseSequences(const char*text,int skipHeader,int dedup,int ambiguous){
 Parsed p={0};unsigned cap=0,line=1,len=0;int fasta=0,haveRecord=0,first=1;
 char*buf=mem((U64)slen(text)+1);
 const unsigned char*it=(const unsigned char*)text;
 if(it[0]==0xEF&&it[1]==0xBB&&it[2]==0xBF)it+=3;
 const unsigned char*look=it;while(*look==' '||*look=='\t'||*look=='\r'||*look=='\n')look++;
 fasta=*look=='>';
 while(*it){
  const unsigned char*start=it;while(*it&&*it!='\n'&&*it!='\r')it++;const unsigned char*end=it;
  if(*it=='\r'){it++;if(*it=='\n')it++;}else if(*it=='\n')it++;
  while(start<end&&(*start==' '||*start=='\t'||*start=='\r'))start++;
  while(end>start&&(end[-1]==' '||end[-1]=='\t'||end[-1]=='\r'))end--;
  if(start==end){line++;continue;}
  if(fasta&&*start=='>'){
   if(haveRecord&&!len){p.error=3;p.errorLine=line;break;}
   appendRecord(&p,buf,len,&cap);len=0;haveRecord=1;line++;continue;
  }
  if(!fasta&&first&&skipHeader){first=0;line++;continue;}first=0;
  if(!fasta)len=0;
  for(const unsigned char*q=start;q<end;q++){
   char c=*q;
   /* Interior tabs usually mean several Excel cells: never join them silently. */
   if(c=='\t'){p.error=4;p.errorLine=line;break;}
   if(c==' ')continue;
   if(c>='a'&&c<='z')c-=32;
   if(!comp(c)||(!ambiguous&&c!='A'&&c!='C'&&c!='G'&&c!='T')){p.error=1;p.errorLine=line;break;}
   buf[len++]=c;
  }
  if(p.error)break;
  if(!fasta){appendRecord(&p,buf,len,&cap);len=0;}line++;
 }
 if(!p.error&&fasta){if(haveRecord&&!len){p.error=3;p.errorLine=line;}else appendRecord(&p,buf,len,&cap);}
 drop(buf);
 if(!p.error&&!p.n)p.error=2;
 if(!p.error&&dedup){
  unsigned size=16;while(size<p.n*2)size*=2;char**table=mem((U64)size*sizeof(char*));unsigned n=0;
  for(unsigned i=0;i<p.n;i++){char*s=p.seq[i];unsigned pos=(unsigned)hash(s)&(size-1);while(table[pos]&&!same(table[pos],s))pos=(pos+1)&(size-1);
   if(table[pos]){drop(s);p.dupes++;}else{table[pos]=s;p.seq[n++]=s;}}
  p.n=n;drop(table);
 }
 return p;
}
static int combinationCount(Parsed*p,unsigned groups,U64*out){U64 n=1;for(unsigned i=0;i<groups;i++){if(!p[i].n||n>(~(U64)0)/p[i].n)return 0;n*=p[i].n;}*out=n;return groups>0;}
static int nextCombination(unsigned*idx,Parsed*p,unsigned groups){for(unsigned k=groups;k>0;k--){unsigned i=k-1;if(++idx[i]<p[i].n)return 1;idx[i]=0;}return 0;}
static char*number(char*b,U64 n){char t[24];unsigned k=0;do{t[k++]=(char)('0'+n%10);n/=10;}while(n);while(k)*b++=t[--k];*b=0;return b;}
static char*textcat(char*b,const char*s){while(*s)*b++=*s++;*b=0;return b;}
static int hexDigit(char c){return (c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f');}
static char*xescape(char*b,const char*s){while(*s){
 if(*s=='&')b=textcat(b,"&amp;");else if(*s=='<')b=textcat(b,"&lt;");else if(*s=='>')b=textcat(b,"&gt;");else if(*s=='\"')b=textcat(b,"&quot;");
 /* Protect literal SpreadsheetML escape sequences in editable column names. */
 else if(s[0]=='_'&&s[1]=='x'&&hexDigit(s[2])&&hexDigit(s[3])&&hexDigit(s[4])&&hexDigit(s[5])&&s[6]=='_')b=textcat(b,"_x005F_");
 else *b++=*s;
 s++;}*b=0;return b;}
static char*column(char*b,unsigned n){char t[8];unsigned k=0;do{t[k++]=(char)('A'+n%26);if(n<26)break;n=n/26-1;}while(1);while(k)*b++=t[--k];*b=0;return b;}
static char*xmlCell(char*b,unsigned col,U64 row,const char*value){b=textcat(b,"<c r=\"");b=column(b,col);b=number(b,row);b=textcat(b,"\" t=\"inlineStr\"><is><t>");b=xescape(b,value);return textcat(b,"</t></is></c>");}
