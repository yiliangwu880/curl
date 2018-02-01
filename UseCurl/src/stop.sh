
#!/bin/sh

## Stop One Module
DoStopServer()
{
    SERVER=$(echo $1 | tr -d '\r\n')
    [ -z "$SERVER" ] && return

    echo -n "Stop $SERVER ..."
	ID=`ps x | grep $SERVER | grep -v grep |  awk  '{print $1}'`
	
	#echo id=$ID
	
	#不是数字就取消
	if [[ $ID =~ ^[0-9]+$ ]]
      then
	  echo kill -9 $ID
    elif [[ $ID =~ ^[A-Za-z]+$ ]]
      then
           echo "error String."
		   exit
     else
           echo "errormixed number and string or others "
		   exit
    fi  
	
	kill -9 $ID
	echo done
	exit
}

DoStopServer UseCurl.bin
