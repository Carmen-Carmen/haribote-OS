#include <stdio.h>
#include <string.h>

char * trim_cmdline_space(char * cmdline, char * cmd) {
    int i = 0, j = 0, end = 0, len = strlen(cmdline);
    static char trimmed[256];

    // 去掉前面的空格
    while (cmdline[i] == ' ') {
        i++;
    }

    // 来到cmd之后的内容
    // 将cmdline的内容拷贝到trimmed
    end = i + strlen(cmd);
    while (i < end) {
        trimmed[j++] = cmdline[i++];
    }

    // cmd与arg之间的空格
    while (i < len && cmdline[i] == ' ') {
        i++;
    }

    // 拷贝参数
    trimmed[j++] = ' ';
    while (i < len && cmdline[i] != ' ') {
        trimmed[j++] = cmdline[i++];
    }
    trimmed[j] = 0x00;

    return trimmed;
}

int main() {
    char *cmdline = "echo    arg";
    char *cmd = "echo";

    printf("\"%s\"\n", cmdline);

    cmdline = trim_cmdline_space(cmdline, cmd);
    printf("\"%s\"\n", cmdline);

    return 0;
}

