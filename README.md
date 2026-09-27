# SequenceCombiner v1.2.1

Windows 64 位离线碱基序列组合工具，支持 2–32 组序列的笛卡尔积。

## 下载和运行

下载 [Windows 程序包](dist/SequenceCombiner_v1.2.1_Windows_x64.zip)，完整解压后双击 `SequenceCombiner.exe`。无需安装 Python 或 Excel。

## 功能

- 默认组名为 Group1、Group2，可自行修改、添加、删除和调整顺序。
- 每组独立选择是否增加反向互补列，默认全部关闭。
- 单列序列粘贴，支持 TXT / FASTA 导入、组内去重和 IUPAC 简并碱基。
- 导出 UTF-16LE 制表符 TXT 和 XLSX；每种组合占一行，各组序列及反向互补序列分别成列。
- 组合总数等于各组有效序列数的乘积；反向互补列不增加组合数量。
- 所有序列处理均在本机完成。

## 示例

`examples/Group1.txt` 和 `examples/Group2.txt` 是按整数编码生成的通用合成测试序列，分别有 498 和 494 条，共 246,012 种组合。它们不包含用户的实验序列，也不代表任何经验证的生物学功能。

## 源码

| 文件 | 用途 |
| --- | --- |
| src/app.c | Windows 界面和输入处理 |
| src/core.h | 解析、去重、笛卡尔积和反向互补 |
| src/exporter.h | TXT / XLSX 导出和保存 |
| src/zip.h | XLSX 的 ZIP/XML 封装 |
| src/win.h | Win32 接口声明 |
| src/build.py、src/elf_to_coff.py | Linux x64 下构建 Windows EXE |
| src/test_core.c、src/test_native.c、src/verify.py | 回归验证 |

## 构建与验证

在 Linux x64 环境中，需要 Python 3、GCC 和支持 i386pep 的 GNU binutils。

```bash
python3 src/build.py
python3 -m pip install openpyxl
python3 src/verify.py /tmp/sequencecombiner-verification-new
```

验证目录必须是新的目录。测试通过 Win32 API 替身执行实际 EXE；不等同于真实 Windows / Excel 桌面测试。程序的原始 v1.2.1 已完成 23 个测试场景及 246,012 行逐行对照，公开版本另使用本仓库合成数据复验。

## 限制

XLSX 单表最多 1,048,575 条组合，单个单元格最多 32,767 字符。本程序使用未压缩 ZIP32，XLSX 文件可能较大，最大约 4 GB。超出 Excel 限制时可仅导出 TXT。TXT 自动分列取决于 Excel 导入设置，直接打开配套 XLSX 可获得已分列的单元格。

公开版 EXE 与已交付的 v1.2.1 完全一致；公开包使用通用合成示例。
