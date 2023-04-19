void HariMain(void) {
    *((char *) 0x00102600) = 0; // 0x00102600存放了磁盘文件信息
    api_end();
}
