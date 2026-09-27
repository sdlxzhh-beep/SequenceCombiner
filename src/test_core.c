#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static void*mem(size_t n){return calloc(1,n?n:1);}static void drop(void*p){free(p);}static void*resize(void*p,size_t n){return realloc(p,n);}
#include "core.h"
static FILE*sink;static void output(const void*p,unsigned n){assert(fwrite(p,1,n,sink)==n);}
#include "zip.h"
int main(int argc,char**argv){
 Parsed a=parseSequences("head\r\n at gc \r\nATGC\r\nAA\n",1,1,0);assert(!a.error&&a.n==2&&a.raw==3&&a.dupes==1&&same(a.seq[0],"ATGC"));releaseParsed(&a);
 a=parseSequences(">one\nAC\nGT\n>two\nAA\n",0,1,0);assert(!a.error&&a.n==2&&same(a.seq[0],"ACGT"));releaseParsed(&a);
 a=parseSequences(">one\n>two\nAA",0,1,0);assert(a.error==3);releaseParsed(&a);
 a=parseSequences("ACGT\nATU",0,1,0);assert(a.error==1&&a.errorLine==2);releaseParsed(&a);
 a=parseSequences("\n\n",0,1,0);assert(a.error==2);releaseParsed(&a);
 a=parseSequences("ACGN",0,1,0);assert(a.error==1);releaseParsed(&a);
 a=parseSequences("ACGN",0,1,1);assert(!a.error);releaseParsed(&a);
 a=parseSequences("AA\nAA",0,0,0);assert(a.n==2);releaseParsed(&a);
 a=parseSequences("ACGT\tTGCA\nAA\tTT",0,1,0);assert(a.error==4&&a.errorLine==1);releaseParsed(&a);
 a=parseSequences(">one\nAC\tGT",0,1,0);assert(a.error==4&&a.errorLine==2);releaseParsed(&a);
 a=parseSequences("\t  AC GT\t\rAA\r\nCC\n",0,1,0);assert(!a.error&&a.n==3&&same(a.seq[0],"ACGT")&&same(a.seq[1],"AA"));releaseParsed(&a);
 a=parseSequences(">one\rAC\rGT\r>two\rAA\r",0,1,0);assert(!a.error&&a.n==2&&same(a.seq[0],"ACGT"));releaseParsed(&a);
 a=parseSequences("\t\t\n\t",0,1,0);assert(a.error==2);releaseParsed(&a);
 a=parseSequences("\xEF\xBB\xBF" "header\rAC\rTT",1,1,0);assert(!a.error&&a.n==2);releaseParsed(&a);
 a=parseSequences("head1\thead2\nAT\tGC",1,1,0);assert(a.error==4&&a.errorLine==2);releaseParsed(&a);
 char escaped[300];xescape(escaped,"A&_x0041_<\"_");assert(same(escaped,"A&amp;_x005F_x0041_&lt;&quot;_"));
 const char *shortEscapes[]={"_","_x","_x0","_x00","_x000","_x0000","_x0000_"};
 for(unsigned i=0;i<7;i++){char*s=copy(shortEscapes[i]);xescape(escaped,s);drop(s);}
 char rc[100];reverseComplement("ACGTRYSWKMBDHVN",rc);assert(same(rc,"NBDHVKMWSRYACGT"));
 for(const char*s="ACGTRYSWKMBDHVN";*s;s++)assert(comp(comp(*s))==*s);
 Parsed p[3];p[0]=parseSequences("ATGC\nAAAA",0,1,0);p[1]=parseSequences("AC\nTG\nCC",0,1,0);p[2]=parseSequences("G\nT",0,1,0);U64 total;assert(combinationCount(p,3,&total)&&total==12);
 unsigned idx[3]={0},n=0;do{assert(idx[0]==n/6&&idx[1]==(n/2)%3&&idx[2]==n%2);n++;}while(nextCombination(idx,p,3));assert(n==12);
 Parsed huge[3]={{.n=0xFFFFFFFFU},{.n=0xFFFFFFFFU},{.n=2}};assert(!combinationCount(huge,3,&total));
 if(argc>1){sink=fopen(argv[1],"wb");assert(sink);Zip z={0};initCRC();xlsxStart(&z);char row[4096],*b=row;b=textcat(b,"<row r=\"1\">");b=xmlCell(b,0,1,"A&组");b=xmlCell(b,1,1,"B");b=xmlCell(b,2,1,"B_RC");b=xmlCell(b,3,1,"C");b=textcat(b,"</row>");zdata(&z,row,b-row);memset(idx,0,sizeof(idx));n=0;do{b=textcat(row,"<row r=\"");b=number(b,n+2);b=textcat(b,"\">");b=xmlCell(b,0,n+2,p[0].seq[idx[0]]);b=xmlCell(b,1,n+2,p[1].seq[idx[1]]);reverseComplement(p[1].seq[idx[1]],rc);b=xmlCell(b,2,n+2,rc);b=xmlCell(b,3,n+2,p[2].seq[idx[2]]);b=textcat(b,"</row>");zdata(&z,row,b-row);n++;}while(nextCombination(idx,p,3));xlsxEnd(&z);assert(!z.failed);assert(fclose(sink)==0);}
 for(int i=0;i<3;i++)releaseParsed(&p[i]);puts("Core tests passed: parsing, errors, duplicates, IUPAC, reverse complement, 3-group Cartesian product, XLSX ZIP writer.");return 0;
}
