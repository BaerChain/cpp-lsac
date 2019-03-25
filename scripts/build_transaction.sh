#!/bin/bash

n=0
bigest=63
ip=0.0.0.0
if [ ${2}x = "x" ]
then
    ip="127.0.0.1:8545"
else
    ip=$2
fi
echo $ip
accounts=`curl -s -d '{"jsonrpc":"2.0","id":16,"method":"eth_accounts","params":[]}' $ip`
#echo "accounts is |" $accounts
sleep 3

account_from=`echo $accounts | jq ".result[2]"`
account_to=`echo $accounts | jq ".result[1]"`
if [ ${1}x = "buildx" ]
then
    
    cmd='{"jsonrpc":"2.0","id":29,"method":"eth_getTransactionCount","params":['${account_from}',"latest"]}'
    res=`curl -s -d $cmd $ip | jq ".result"`
    echo $res
    n=$((16#${res:3:-1}))
    bigest=`expr ${n} + ${bigest}`
    begin=$n
    echo $n
    while [ $n -le $bigest ]
    do
        hexBlockNumber=`echo "obase=16;${n}"|bc`
        cmd='{"jsonrpc":"2.0","id":31,"method":"personal_unlockAccount","params":['${account_from}',"123456",600]}'
        res=`curl -s -d $cmd $ip | jq ".result"`
        if [ ${res}x = "truex" ]
        then
            cmd='{"jsonrpc":"2.0","id":25,"method":"eth_signTransaction","params":[{"data":"","from":'${account_from}',"gas":"0x5208","gasPrice":"0x4a817c800","nonce":"0x'${hexBlockNumber}'","to":'${account_to}',"value":"0x1"}]}'
            res=`curl -s -d $cmd $ip | jq ".result.raw"`
            echo $res
        fi

        if [ ${n} = ${begin} ]
        then
            echo $res > 1.txt
        else
            echo $res >> 1.txt
        fi
        n=`expr ${n} + 1`
    done
elif [ ${1}x = "commitx" ]
then
    while read line
    do
        cmd='{"jsonrpc":"2.0","id":31,"method":"personal_unlockAccount","params":['${account_from}',"123456",600]}'
        res=`curl -s -d $cmd $ip | jq ".result"`
        cmd='{"jsonrpc":"2.0","id":12,"method":"eth_sendRawTransaction","params":['${line}']}'
        res=`curl -s -d $cmd $ip | jq ".result"`
        echo $res
    done < 1.txt
fi