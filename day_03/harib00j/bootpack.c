void HariMain(void) {
    fin: // 是C语言中的标签，可以用于goto语法 
        io_hlt(); /*执行naskfunc.asm里的_io_hlt*/
        goto fin;
}
