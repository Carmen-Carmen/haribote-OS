cp `ls -A | grep -v "*.sh.*"` /Volumes/\[C\]\ Windows\ 11/Users/xulingrui/Downloads/30dayOS/30dayOS-master/tolset/haribote



while [ $? -ne 0 ]
do
    open smb://Windows\ 11._smb._tcp.local
    sleep 5
    cp `ls -A | grep -v "*.sh.*"` /Volumes/\[C\]\ Windows\ 11/Users/xulingrui/Downloads/30dayOS/30dayOS-master/tolset/haribote
done
echo "copy succeeded!"
