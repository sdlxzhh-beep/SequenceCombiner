/* TXT uses Unicode tab-delimited text. TXT export also writes a companion
   workbook, so column layout does not depend on Excel's text-import settings. */
static int writeResult(Parsed*parsed,U64 total,unsigned longest,unsigned maxSum,
                       int xlsx,const WCHAR*path,int mayReplace){
 WCHAR*temp=mem(((SIZE_T)wlen(path)+16)*2);wcat(wcat(temp,path),W(".partial"));
 outputHandle=CreateFileW(temp,0x40000000,0,0,1,0x80,0);
 if(outputHandle==(HANDLE)-1){drop(temp);return -1;}
 ioOK=1;U64 completed=0;unsigned idx[32]={0};Zip z={0};initCRC();
 SIZE_T capacity=(SIZE_T)maxSum*6+groupCount*1200+8192;
 char*row=mem(capacity),*rc=mem((SIZE_T)longest+1),*names[32]={0};
 WCHAR*unicodeRow=xlsx?0:mem(capacity*2);
 for(unsigned i=0;i<groupCount;i++)names[i]=utf8(groups[i].name);
 if(xlsx)xlsxStart(&z);else output("\xFF\xFE",2);
 char*b=row;unsigned col=0;if(xlsx)b=textcat(b,"<row r=\"1\">");
 for(unsigned i=0;i<groupCount;i++){
  char head[400];textcat(head,names[i]);
  if(xlsx)b=xmlCell(b,col++,1,head);else{if(i)*b++='\t';b=textcat(b,head);}
  if(groups[i].rc){textcat(head+slen(head),"_RC");if(xlsx)b=xmlCell(b,col++,1,head);else{*b++='\t';b=textcat(b,head);}}
 }
 if(xlsx){b=textcat(b,"</row>");zdata(&z,row,(unsigned)(b-row));}
 else{b=textcat(b,"\r\n");WCHAR*header=wide(row);if(!header)ioOK=0;else{output(header,wlen(header)*2);drop(header);}}
 while(completed<total&&ioOK&&!cancelled&&!z.failed){
  b=row;col=0;if(xlsx){b=textcat(b,"<row r=\"");b=number(b,completed+2);b=textcat(b,"\">");}
  for(unsigned i=0;i<groupCount;i++){
   const char*s=parsed[i].seq[idx[i]];
   if(xlsx)b=xmlCell(b,col++,completed+2,s);else{if(i)*b++='\t';b=textcat(b,s);}
   if(groups[i].rc){reverseComplement(s,rc);if(xlsx)b=xmlCell(b,col++,completed+2,rc);else{*b++='\t';b=textcat(b,rc);}}
  }
  if(xlsx){b=textcat(b,"</row>");zdata(&z,row,(unsigned)(b-row));}
  else{b=textcat(b,"\r\n");unsigned n=(unsigned)(b-row);for(unsigned j=0;j<n;j++)unicodeRow[j]=(unsigned char)row[j];output(unicodeRow,n*2);}
  completed++;nextCombination(idx,parsed,groupCount);if(completed%1000==0)pump(completed,total);
 }
 if(xlsx&&ioOK&&!cancelled&&!z.failed)xlsxEnd(&z);
 if(!FlushFileBuffers(outputHandle))ioOK=0;
 if(!CloseHandle(outputHandle))ioOK=0;
 int success=ioOK&&!cancelled&&!z.failed&&completed==total;
 if(success)success=MoveFileExW(temp,path,8|(mayReplace?1:0));
 if(!success)DeleteFileW(temp);
 for(unsigned i=0;i<groupCount;i++)drop(names[i]);
 drop(row);drop(rc);drop(unicodeRow);drop(temp);
 return success?1:(cancelled?-2:-1);
}
/* Create a unique sibling name; never silently replace an existing workbook. */
static WCHAR*companionPath(const WCHAR*path){
 unsigned n=wlen(path);if(n>32000)return 0;
 unsigned stem=n;for(unsigned i=n;i>0;i--){WCHAR c=path[i-1];if(c=='/'||c=='\\')break;if(c=='.'){stem=i-1;break;}}
 WCHAR*result=mem(65536);bytes(result,path,(SIZE_T)stem*2);
 for(unsigned count=1;count<=10000;count++){
  WCHAR*p=result+stem;p=wcat(p,W("_Excel"));
  if(count>1){char numberBuf[24];number(numberBuf,count);WCHAR*w=wide(numberBuf);p=wcat(p,W("_"));p=wcat(p,w);drop(w);}
  wcat(p,W(".xlsx"));if(GetFileAttributesW(result)==0xFFFFFFFFU)return result;
 }
 drop(result);return 0;
}
static int correctExtension(const WCHAR*path,int xlsx){
 const WCHAR*ext=xlsx?W(".xlsx"):W(".txt");unsigned n=wlen(path),m=wlen(ext);if(n<m)return 0;
 for(unsigned i=0;i<m;i++){WCHAR c=path[n-m+i];if(c>='A'&&c<='Z')c+=32;if(c!=ext[i])return 0;}return 1;
}
static void exportFile(int xlsx){
 Parsed parsed[32]={0};U64 total;
 if(!prepare(parsed,&total)){freeAll(parsed);return;}
 unsigned longest=0,maxSum=0,columns=0;
 for(unsigned i=0;i<groupCount;i++){unsigned m=0;for(unsigned j=0;j<parsed[i].n;j++){unsigned n=slen(parsed[i].seq[j]);if(n>m)m=n;}if(m>longest)longest=m;maxSum+=m*(groups[i].rc?2:1);columns+=groups[i].rc?2:1;}
 const char*excelLimit=0;
 if(total>1048575)excelLimit="组合数超过 XLSX 单表 1,048,575 行的数据上限。";
 else if(longest>32767)excelLimit="有序列超过 Excel 单元格 32,767 字符限制。";
 else if(total>4000000000ULL/((U64)maxSum+columns*110ULL+80))excelLimit="预计 XLSX 超过本程序 4 GB 文件上限。";
 if(xlsx&&excelLimit){warn(excelLimit);freeAll(parsed);return;}
 if(!xlsx&&excelLimit){char msg[600];textcat(textcat(msg,excelLimit),"本次仅导出 TXT。是否继续？");WCHAR*w=wide(msg);int answer=MessageBoxW(window,w,W("XLSX 导出限制"),0x24);drop(w);if(answer!=6){freeAll(parsed);return;}}
 if(total>1000000&&MessageBoxW(window,W("组合数超过 100 万，导出可能耗时较长并占用较多磁盘空间。是否继续？"),W("较大的组合表"),0x24)!=6){freeAll(parsed);return;}
 WCHAR*path=mem(65536);wcat(path,xlsx?W("sequence_combinations.xlsx"):W("sequence_combinations.txt"));
 if(!chooseFile(path,1,xlsx)){drop(path);freeAll(parsed);return;}
 if(!correctExtension(path,xlsx)){warn(xlsx?"导出 XLSX 的文件名必须以 .xlsx 结尾，请重新选择保存名称。":"导出 TXT 的文件名必须以 .txt 结尾，请重新选择保存名称。");drop(path);freeAll(parsed);return;}
 WCHAR*excelPath=0;
 if(!xlsx&&!excelLimit){excelPath=companionPath(path);if(!excelPath){warn("无法生成配套 XLSX 文件名，请更换保存位置或使用较短的文件名。");drop(path);freeAll(parsed);return;}}
 setBusy(1);cancelled=0;
 int first=writeResult(parsed,total,longest,maxSum,xlsx,path,1),second=0;
 if(first==1&&excelPath){SetWindowTextW(status,W("TXT 已保存，正在生成已分列的 Excel 工作簿……"));second=writeResult(parsed,total,longest,maxSum,1,excelPath,0);}
 setBusy(0);
 if(first!=1){SetWindowTextW(status,first==-2?W("已取消导出。"):W("导出失败。请检查磁盘空间、文件权限或目标文件占用。"));if(first!=-2)warn("导出失败，目标文件未被替换。请检查磁盘空间、文件权限或是否有同名 .partial 文件。");}
 else{
  WCHAR*message=mem(((SIZE_T)wlen(path)+(excelPath?wlen(excelPath):0)+500)*2);WCHAR*p=message;
  if(excelPath&&second==1){p=wcat(p,W("已生成两个文件。查看分列结果，请直接打开下面的 XLSX：\r\n"));p=wcat(p,excelPath);p=wcat(p,W("\r\n\r\nTXT 文本：\r\n"));wcat(p,path);SetWindowTextW(status,W("TXT 和 XLSX 均已完成。打开配套 XLSX 即可直接查看分列结果。"));}
  else if(excelPath){p=wcat(p,second==-2?W("TXT 已保存。配套 XLSX 的生成已取消。\r\n"):W("TXT 已保存，但配套 XLSX 生成失败。请检查磁盘空间、权限或同名临时文件。\r\n"));wcat(p,path);SetWindowTextW(status,W("仅 TXT 已保存，配套 XLSX 尚未生成。"));}
  else{p=wcat(p,xlsx?W("Excel 工作簿已保存，组合已经按列写入单元格：\r\n"):W("TXT 已保存。本次数据超出 Excel 导出限制，未生成 XLSX：\r\n"));wcat(p,path);SetWindowTextW(status,W("导出完成。"));}
  MessageBoxW(window,message,W("导出结果"),64);drop(message);
 }
 drop(excelPath);drop(path);freeAll(parsed);
}
