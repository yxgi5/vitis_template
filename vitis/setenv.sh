#!/bin/bash

## assume you have Vivado 2020.1 (and Vitis) installed in D:\Xilinx\, which is necessary

# you need one of bash.exe from following way
## assume you have msys64 installed in D:\msys64\, offering bash.exe
## assume you have Git installed in C:\Program Files\, offering bash.exe(not git-bash.exe)
## assume you have cygwin64 installed in D:\cygwin64\, offering bash.exe

# you can get OSTYPE by command `bash -c "echo $OSTYPE"`
# here is an avilable list and coresponding OS
# darwin*       # Mac OS
# darwin9       # iOS
# solaris*      # Solaris
# linux-gnu     # Android
# linux-android # Android (termux) 
# linux-gnu     # Linux
# cygwin        # CYGWIN
# msys          # MSYS2
# linux-gnu     # Win10 bash
# openbsd*      # OpenBSD
# FreeBSD       # FreeBSD
# netbsd        # NetBSD
# ...


# open bash.exe and 
# DIR=`cygpath -ua "D:\path\to\project"` && cd $DIR
# source setenv.sh

query_vars_msys() {
  (cmd.exe /c "call D:\Xilinx\Vitis\2020.1\settings64.bat && D:\Xilinx\Vivado\2020.1\tps\win64\git-2.16.2\bin\bash.exe -c '/usr/bin/printenv PATH'")
}

query_vars_cygwin() {
  (cmd.exe /c "call D:\Xilinx\Vitis\2020.1\settings64.bat && D:\cygwin64\bin\bash.exe -c '/usr/bin/printenv PATH'")
}

echo -e "\033[42;31mScript ver 0.1.002 \nMake sure the env path is correct before running!! \033[0m"

#if [[ "$OSTYPE" == "linux-gnu"* ]]; then
#    source /opt/Xilinx/Vitis/2020.1/settings64.sh
#fi

#case "$OSTYPE" in
#solaris*) echo "SOLARIS" ;;
#darwin*) echo "OSX" ;;
#linux*) echo "LINUX" ;;
#bsd*) echo "BSD" ;;
#msys*) echo "WINDOWS" ;;
#cygwin*) echo "ALSO WINDOWS" ;;
#*) echo "unknown: $OSTYPE" ;;
#esac

#case "$OSTYPE" in
#    solaris*) 
#        echo "SOLARIS" 
#        ;;
#    darwin*) 
#        echo "OSX" 
#        ;;
#    linux*) 
#        echo "LINUX" 
#        ;;
#    bsd*) 
#        echo "BSD" 
#        ;;
#    msys*) 
#        echo "MSYS2" 
#        ;;
#    cygwin*) 
#        echo "CYGWIN" 
#        ;;
#    *) 
#        echo "unknown: $OSTYPE" 
#        ;;
#esac

if [[ "$SETENV" -eq "" ]]; then
    echo "SETENV has not been set before, setting ..."

    case "$OSTYPE" in
        linux*) 
            echo "OSTYPE = $OSTYPE"
            source /opt/Xilinx/Vitis/2020.1/settings64.sh
            export SETENV=1
            ;;
        msys*) 
            echo "OSTYPE = $OSTYPE"
            #export PATH="$PATH:$(query_vars_msys):/c/Users/dengl/AppData/Roaming/MobaXterm/slash/bin/"
            export PATH="$PATH:$(query_vars_msys)"
            if [ $? != 0 ]
            then
                echo -e "\n"
                echo -e "\033[41;36m setting vars fail!!! \033[0m"
                exit 1
            fi
            alias xmd=xmd.bat
            alias xsct=xsct.bat
            alias xsdb=xsdb.bat
            export SETENV=1
            ;;
        cygwin*) 
            echo "OSTYPE = $OSTYPE"
            #export PATH="$PATH:$(query_vars_cygwin):/cygdrive/c/Users/dengl/AppData/Roaming/MobaXterm/slash/bin/"
            export PATH="$PATH:$(query_vars_cygwin)"
            if [ $? != 0 ]
            then
                echo -e "\n"
                echo -e "\033[41;36m setting vars fail!!! \033[0m"
                exit 1
            fi
            alias xmd=xmd.bat
            alias xsct=xsct.bat
            alias xsdb=xsdb.bat
            export SETENV=1
            ;;
        *) 
            #echo "unknown: $OSTYPE" 
            echo -e "\033[41;36m OSTYPE Unkown: $OSTYPE \033[0m"
            exit 1
            ;;
    esac
else
    echo SETENV=$SETENV
fi

#xsct

