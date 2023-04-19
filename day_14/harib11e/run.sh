./cp_to_linux.sh
sleep 20
scp ubuntu_x86_64:/home/xulingrui/.wine/drive_c/users/xulingrui/Downloads/30dayOS-master/tolset/haribote/haribote.img ~/Downloads
if [ $? -eq 0 ];then
    qemu-system-i386 -fda ~/Downloads/haribote.img -m 32
else
    echo "[error]: no image made!"
fi
