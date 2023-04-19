# copy source files to tolset directory
# ./cp_to_linux.sh
# ./cp_to_win.sh
./cp_to_tolset.sh

# wait for a while
# sleep 12

# copy the image file from tolset directory to ~/Downloads
# cp from linux
# scp ubuntu_x86_64:/home/xulingrui/.wine/drive_c/users/xulingrui/Downloads/30dayOS-master/tolset/haribote/haribote.img ~/Downloads

# cp from win
# cp /Volumes/\[C\]\ Windows\ 11/Users/xulingrui/Downloads/30dayOS/30dayOS-master/tolset/haribote/haribote.img ~/Downloads

sleep 5
found=0
for i in {1..15}
do
    sleep 1
    # cp from local tolset
    cp ~/Downloads/OS/30dayOS/30dayOS-master/tolset/haribote/haribote.img ~/Downloads 2>/dev/null
    if [ $? -eq 0 ];then
        found=1
        break
    else
        waited=$((5 + i))
        echo "waited $waited s"
    fi
done

# run qemu
if [ $found -eq 1 ];then
    qemu-system-i386 -fda ~/Downloads/haribote.img -m 32
else
    echo "[error]: no image made!"
fi
