# ./cp_to_linux.sh
# ./cp_to_win.sh
./cp_to_tolset.sh

sleep 8

# cp from linux
# scp ubuntu_x86_64:/home/xulingrui/.wine/drive_c/users/xulingrui/Downloads/30dayOS-master/tolset/haribote/haribote.img ~/Downloads

# cp from win
# cp /Volumes/\[C\]\ Windows\ 11/Users/xulingrui/Downloads/30dayOS/30dayOS-master/tolset/haribote/haribote.img ~/Downloads

# cp from local tolset
cp ~/Downloads/30dayOS-master/tolset/haribote/haribote.img ~/Downloads

# run qemu
if [ $? -eq 0 ];then
    qemu-system-i386 -fda ~/Downloads/haribote.img -m 32
else
    echo "[error]: no image made!"
fi
