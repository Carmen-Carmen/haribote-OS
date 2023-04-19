FLAG="run.sh"

while true; do
    if [ -e "$FLAG" ]; then
# 如果找到run.sh，就make img
        rm "$FLAG"
        start_time=$(date +%s)
        # wine cmd < clean.bat 2>/dev/null
        # wine cmd < make_img.bat 2>/dev/null
        wine cmd < full_make.bat 2>log

        if [ -e "haribote.img" ]; then            
            end_time=$(date +%s)
            run_time=$((end_time - start_time))
            echo "******************************"
            echo "RunTime: $run_time s"
            echo "******************************"
            echo "" > log
        else
            cat log | grep error
            cat log | grep Error
        fi
    fi
    sleep 1
done


