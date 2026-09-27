"""Translate x64 freestanding ELF relocatable objects to AMD64 COFF.
Preserves explicit RELA addends and symbol indices, including section symbols.
Only the relocation types used by the bundled freestanding C build are accepted.
"""
import struct
from pathlib import Path
def convert(source,destination):
    data=Path(source).read_bytes()
    assert data[:6]==b'\x7fELF\x02\x01'
    h=struct.unpack_from('<16sHHIQQQIHHHHHH',data)
    assert h[1]==1 and h[2]==62
    sections=[struct.unpack_from('<IIQQQQIIQQ',data,h[6]+i*h[11]) for i in range(h[12])]
    names=sections[h[13]]; strings=data[names[4]:names[4]+names[5]]
    def cstr(blob,i): return blob[i:blob.index(0,i)].decode()
    selected=[i for i,s in enumerate(sections) if s[2]&2 and s[1] in (1,8)]
    mapping={old:i+1 for i,old in enumerate(selected)}
    symindex=next(i for i,s in enumerate(sections) if s[1]==2)
    symsec=sections[symindex]; st=sections[symsec[6]]; strs=data[st[4]:st[4]+st[5]]
    syms=[struct.unpack_from('<IBBHQQ',data,symsec[4]+i*24) for i in range(symsec[5]//24)]
    stringtable=bytearray(b'\0'*4)
    def namebytes(name,section=False):
        b=name.encode()
        if len(b)<=8:return b.ljust(8,b'\0')
        off=len(stringtable);stringtable.extend(b+b'\0')
        return ('/'+str(off)).encode().ljust(8,b'\0') if section else struct.pack('<II',0,off)
    symbolmap={}; symbols=bytearray()
    for old,s in enumerate(syms):
        n,info,other,si,value,size=s
        if old==0 or si not in (0,0xFFF1) and si not in mapping:continue
        nm=cstr(strs,n) if n else cstr(strings,sections[si][0])
        symbolmap[old]=len(symbols)//18
        symbols.extend(struct.pack('<8sIhHBB',namebytes(nm),value,-1 if si==0xFFF1 else mapping.get(si,0),0x20 if info&15==2 else 0,2 if info>>4 else 3,0))
    bodies={i:bytearray(data[s[4]:s[4]+s[5]]) if s[1]!=8 else bytearray() for i,s in enumerate(sections) if i in mapping}
    relocs={i:bytearray() for i in selected}
    for s in sections:
        if s[1]!=4 or s[7] not in mapping:continue
        target=s[7]; body=bodies[target]
        for off in range(s[4],s[4]+s[5],24):
            addr,info,addend=struct.unpack_from('<QQq',data,off);typ=info&0xFFFFFFFF;symbol=info>>32
            assert symbol in symbolmap
            if typ==1: kind=1;struct.pack_into('<Q',body,addr,addend&0xFFFFFFFFFFFFFFFF)
            elif typ in (2,4):kind=4;struct.pack_into('<I',body,addr,(addend+4)&0xFFFFFFFF)
            elif typ in (10,11):kind=2;struct.pack_into('<I',body,addr,addend&0xFFFFFFFF)
            else:raise ValueError(f'Unsupported relocation {typ}')
            relocs[target].extend(struct.pack('<IIH',addr,symbolmap[symbol],kind))
    pos=20+40*len(selected); contents=bytearray(); headers=bytearray()
    for i in selected:
        s=sections[i];body=bodies[i];rels=relocs[i];rawptr=pos if body else 0
        contents.extend(body);pos+=len(body);relptr=pos if rels else 0;contents.extend(rels);pos+=len(rels)
        alignment=max(1,s[8]);power=min(13,alignment.bit_length());flags=power<<20
        flags|=0x60000020 if s[2]&4 else (0xC0000000 if s[2]&1 else 0x40000000)|(0x80 if s[1]==8 else 0x40)
        assert len(rels)//10<65536
        headers.extend(struct.pack('<8sIIIIIIHHI',namebytes(cstr(strings,s[0]),True),0,0,s[5],rawptr,relptr,0,len(rels)//10,0,flags))
    struct.pack_into('<I',stringtable,0,len(stringtable))
    head=struct.pack('<HHIIIHH',0x8664,len(selected),0,pos,len(symbols)//18,0,0)
    Path(destination).write_bytes(head+headers+contents+symbols+stringtable)
if __name__=='__main__':
    import sys
    convert(sys.argv[1],sys.argv[2])
