/* Streaming ZIP32 writer (STORE), with CRC32 and an explicit central directory.
   Caller supplies output(data,length), and checks its I/O result. */
typedef struct{char name[96];unsigned offset,size,crc;}ZipPart;
typedef struct{ZipPart parts[12];unsigned count,current;U64 pos;unsigned crc,size;int failed;}Zip;
static unsigned crcTable[256];
static void initCRC(void){for(unsigned i=0;i<256;i++){unsigned c=i;for(int k=0;k<8;k++)c=(c&1)?0xEDB88320U^(c>>1):c>>1;crcTable[i]=c;}}
static void zraw(Zip*z,const void*p,unsigned n){if(z->pos+n>0xFFFFFFFFULL){z->failed=1;return;}output(p,n);z->pos+=n;}
static void z16(Zip*z,unsigned n){unsigned char b[2]={(unsigned char)n,(unsigned char)(n>>8)};zraw(z,b,2);}
static void z32(Zip*z,unsigned n){unsigned char b[4]={(unsigned char)n,(unsigned char)(n>>8),(unsigned char)(n>>16),(unsigned char)(n>>24)};zraw(z,b,4);}
static void zbegin(Zip*z,const char*name){ZipPart*p=&z->parts[z->count++];textcat(p->name,name);p->offset=(unsigned)z->pos;z->crc=~0U;z->size=0;
 z32(z,0x04034B50);z16(z,20);z16(z,8);z16(z,0);z16(z,0);z16(z,0x5C21);z32(z,0);z32(z,0);z32(z,0);z16(z,slen(name));z16(z,0);zraw(z,name,slen(name));}
static void zdata(Zip*z,const void*data,unsigned n){const unsigned char*p=data;for(unsigned i=0;i<n;i++)z->crc=crcTable[(z->crc^p[i])&255]^(z->crc>>8);z->size+=n;zraw(z,data,n);}
static void ztext(Zip*z,const char*s){zdata(z,s,slen(s));}
static void zend(Zip*z){ZipPart*p=&z->parts[z->count-1];p->size=z->size;p->crc=z->crc^~0U;z32(z,0x08074B50);z32(z,p->crc);z32(z,p->size);z32(z,p->size);}
static void zfile(Zip*z,const char*n,const char*s){zbegin(z,n);ztext(z,s);zend(z);}
static void zclose(Zip*z){unsigned start=(unsigned)z->pos;for(unsigned i=0;i<z->count;i++){ZipPart*p=&z->parts[i];z32(z,0x02014B50);z16(z,20);z16(z,20);z16(z,8);z16(z,0);z16(z,0);z16(z,0x5C21);z32(z,p->crc);z32(z,p->size);z32(z,p->size);z16(z,slen(p->name));z16(z,0);z16(z,0);z16(z,0);z16(z,0);z32(z,0);z32(z,p->offset);zraw(z,p->name,slen(p->name));}unsigned size=(unsigned)z->pos-start;z32(z,0x06054B50);z16(z,0);z16(z,0);z16(z,z->count);z16(z,z->count);z32(z,size);z32(z,start);z16(z,0);}
static void xlsxStart(Zip*z){
 zfile(z,"[Content_Types].xml","<?xml version=\"1.0\" encoding=\"UTF-8\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"><Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/><Default Extension=\"xml\" ContentType=\"application/xml\"/><Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/><Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/></Types>");
 zfile(z,"_rels/.rels","<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/></Relationships>");
 zfile(z,"xl/workbook.xml","<?xml version=\"1.0\" encoding=\"UTF-8\"?><workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><sheets><sheet name=\"Combinations\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>");
 zfile(z,"xl/_rels/workbook.xml.rels","<?xml version=\"1.0\" encoding=\"UTF-8\"?><Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"><Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/></Relationships>");
 zbegin(z,"xl/worksheets/sheet1.xml");
 ztext(z,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetViews><sheetView workbookViewId=\"0\"><pane ySplit=\"1\" topLeftCell=\"A2\" activePane=\"bottomLeft\" state=\"frozen\"/></sheetView></sheetViews><cols><col min=\"1\" max=\"64\" width=\"46\" customWidth=\"1\"/></cols><sheetData>");
}
static void xlsxEnd(Zip*z){ztext(z,"</sheetData></worksheet>");zend(z);zclose(z);}
