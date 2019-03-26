#!/bin/bash

n=0
bigest=9999
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
isFull=`echo $accounts | jq ".result[$bigest]"`
vote=`echo $accounts | jq ".result[0]"`
if [ ${1}x = "initx" ]
then
    while [ ${isFull}x = "nullx" ]
    do
        while [ $n -le $bigest ]
        do
            account=`echo $accounts | jq ".result[$n]"`
            if [ $account = "null" ]
            then
                newAccount=`curl -s -d '{"jsonrpc":"2.0","id":15,"method":"personal_newAccount","params":["123456"]}' $ip`
                sleep 1
                echo $n $newAccount
            #else
                #echo $n $account
            fi
            n=`expr ${n} + 1`
        done
        accounts=`curl -s -d '{"jsonrpc":"2.0","id":16,"method":"eth_accounts","params":[]}' $ip`
        isFull=`echo $accounts | jq ".result[$bigest]"`
        n=0
    done

    n=0
    accounts=`curl -s -d '{"jsonrpc":"2.0","id":13,"method":"eth_accounts","params":[]}' $ip`
    while [ $n -le $bigest ]
    do
        account=`echo $accounts | jq ".result[$n]"`
        if [ ${account}x != "x" ]
        then
            if [ ${account}x != "nullx" ]
            then
                cmd='{"jsonrpc":"2.0","id":9,"method":"personal_sendTransaction","params":[{"from":'${account}',"to":'${vote}',"type":3,"value":"0x1"},"123456"]}'
                tr=`curl -s -d $cmd $ip`
                echo $cmd
            fi
        fi
        n=`expr ${n} + 1`
    done
elif [ ${1}x = "calcx" ]
then
    cmd='{"jsonrpc":"2.0","id":15,"method":"eth_blockNumber","params":[]}'
    res=`curl -s -d $cmd $ip | jq ".result"`
    echo $res
    beforeBlockNumber=$((16#${res:3:-1}+1))
    cmd='{"jsonrpc":"2.0","id":12,"method":"miner_setEtherbase","params":['${vote}']}'
    res=`curl -s -d $cmd $ip | jq ".result"`
    if [ ${res}x = "truex" ]
    then
        cmd='{"jsonrpc":"2.0","id":13,"method":"miner_start","params":[1]}'
        res=`curl -s -d $cmd $ip | jq ".result"`
        if [ ${res}x != "truex" ]
        then
            exit
        fi
        sleep 2
        cmd='{"jsonrpc":"2.0","id":14,"method":"miner_stop","params":[]}'
        res=`curl -s -d $cmd $ip | jq ".result"`
        while [ ${res}x != "truex" ]
        do
            res=`curl -s -d $cmd $ip | jq ".result"`
        done
        cmd='{"jsonrpc":"2.0","id":15,"method":"eth_blockNumber","params":[]}'
        res=`curl -s -d $cmd $ip | jq ".result"`
        echo $res
        afterBlockNumber=$((16#${res:3:-1}))
        sumTransaction=0
        while [ $beforeBlockNumber -le $afterBlockNumber ]
        do
            hexBlockNumber=`echo "obase=16;${beforeBlockNumber}"|bc`
            cmd='{"jsonrpc":"2.0","id":35,"method":"eth_getBlockTransactionCountByNumber","params":["0x'${hexBlockNumber}'"]}'
            res=`curl -s -d $cmd $ip | jq ".result"`
            sumTransaction=$((16#${res:3:-1}+${sumTransaction}))
            beforeBlockNumber=`expr ${beforeBlockNumber} + 1`
            echo "total transaction is" $sumTransaction
        done
    fi
    echo "res " $res
    
fi