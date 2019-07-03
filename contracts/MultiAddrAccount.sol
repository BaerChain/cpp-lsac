pragma solidity ^0.4.25;

import "./01_1_AccountFactory.sol";
import "./01_6_Token.sol";

contract MultiAddrAccount is AccountFactory, TokenBRC {
    
    event AddAccountAddr(string account, address newAddr);
    event RemoveAccountAddr(string account, address removeAddr);


    // 判断地址是否有效
    modifier isAddrValid(address addr_1, address addr_2) {
        require(addr_1 != address(0) && addr_2 != address(0) && addr_1 != addr_2, "This is invalid address!");
        _;
    }
    
    // 判断地址是否为空
    modifier notNull(address _address) {
        require(_address != 0, "This address is null!");
        _;
    }
    
    // 判断是否为账户的超级地址
    modifier onlyAccountRoot(string account) {
        require(msg.sender == accountAddrs[account][0], "This address is not accountRoot!");
        _;
    }
    
    // 地址不存在
    modifier addrDoesNotExists(address addr) {
        require(!isAccountAddr[addr], "This newAddr has been exists!");
        _;
    }
    
    // 地址存在 
    modifier addrExists(address addr) {
        require(isAccountAddr[addr], "This removeAddr does not exists!");
        _;
    }
    
    // 地址不能为超级地址
    modifier notAccountRoot(string account, address addr) {
        require(addr != accountAddrs[account][0], "This addr is the accountRoot!");
        _;
    }
    
    // 判断地址不为空
    modifier notFull(address addr) {
        require(addr != address(0), "This addr is full!");
        _;
    }
    
    //注册账户
    function registerMultiAddrAccount(string account, address addr_1, address addr_2) 
        public 
        isAddrValid(addr_1, addr_2)
    {
        register(account, addr_1, addr_2);
    }
    
    // 添加新的地址
    function addAccountAddr(string account, address newAddr) 
        public
        onlyAccountRoot(account)
        addrDoesNotExists(newAddr)
        notFull(newAddr)
    {
        isAccountAddr[newAddr] = true;
        accountAddrs[account].push(newAddr);
        addrAccount[newAddr] = account;
        
        emit AddAccountAddr(account, newAddr);
    }
    
    // 移除地址
    function removeAccountAddr(string account, address removeAddr) 
        public
        notAccountRoot(account, removeAddr)
        onlyAccountRoot(account)
        addrExists(removeAddr)
    {
        isAccountAddr[removeAddr] = false;
        for(uint i = 0; i < accountAddrs[account].length - 1; i++) {
            if(accountAddrs[account][i] == removeAddr) {
                accountAddrs[account][i] = accountAddrs[account][accountAddrs[account].length - 1];
                break;
            }
        }
        accountAddrs[account].length -= 1;
        emit RemoveAccountAddr(account, removeAddr);
    }
        
    function approveAndCall(address destination, uint value, uint dataLength, bytes data) 
        public 
        notNull(destination)
        returns(bool) 
    {
        if(external_call(destination, value, dataLength, data)) {
            return true;
        } else {
            return false;
        }
    }

    function external_call(address destination, uint value, uint dataLength, bytes data) 
        private 
        returns(bool) 
        {
            bool result;
            assembly {
                let x := mload(0x40) 
                let d := add(data, 32) 
                result := call(
                    sub(gas, 34710),
                    destination,
                    value,
                    d,
                    dataLength,
                    x,
                    0)
        }
        return result;
     }
    
    //通过地址获取账户
    function getAddrAccountInfo(address addr) public view returns(string) {
        return addrAccount[addr];
    }
}
