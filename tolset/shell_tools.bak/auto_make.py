import os
import time

if __name__ == "__main__": 
    path = "." + os.sep
    # 遍历当前文件夹下是否存在run.sh，如果有就说明cp_to_win.sh已经执行
    while True: 
        time.sleep(1)
        isFlag = False

        # for file_path, sub_dirs, filenames in os.walk(path): 
        #     if filenames:
        #         for filename in filenames: 
        #             if filename == "run.sh": 
        #                 isFlag = True
        #                 break

        for filename in os.listdir():
            if filename == 'run.sh': 
                isFlag = True
                break

        if isFlag: 
            start = time.perf_counter()
            os.system(path + "full_make.bat")
            end = time.perf_counter()
            run_time = end - start
            print("\n****************")
            print("runtime: %.2f s" % run_time)
            print("****************")
            isFlag = False
