vivado 2020.1 + msys2


win里仅使用
compare_src.bat	## 最好先执行
proj_gen.bat	## 会删除workspace，所以执行之前要备份
proj_build.bat	## 不会删除workspace，如果没有就会新建workspace
post_build.bat
flash.bat
win里用 setenv_bash.bat 调用clean.sh, 有这功夫手动删不行吗
cean.bat 		##仅做安全删除, 不删除workspace

linux仅使用
proj_gen.sh
proj_build.sh
post_build.sh
flash.sh
clean.sh


cmd 	是docker容器ubuntu的快捷操作命令




根据需要和目标文件大小来分配存储区，地址只能是multiple of 32K
比如，镜像下载到两个地址，如果0x0开始的镜像损坏，就可以从0x5000000加载镜像
0x0
0x5000000

上面脚本默认zynqmp工程，zynq工程需进行必要修改
-------------------------------------------------------------------

microblaze ram on chip 工程的post_build脚本，实际用的时候替换有关脚本
post_build_mb0.sh
post_build_mb0.bat

microblaze ram on chip 工程的flash脚本，实际用的时候替换有关脚本
flash_mb0.sh
flash_mb0.bat

microblaze ram on ddr 工程的去参考 microblaze git




