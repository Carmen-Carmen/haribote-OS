./cp_to_win.sh
sleep 8
cp /Volumes/\[C\]\ Windows\ 11/Users/xulingrui/Downloads/30dayOS/30dayOS-master/tolset/haribote/haribote.img ~/Downloads
if [ $? -eq 0 ];then
    qemu-system-x86_64 -fda ~/Downloads/haribote.img -m 32
else
    echo "[error]: no image made!"
fi
