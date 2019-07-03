pragma solidity ^0.4.25;

contract AccountFactory {
    event AccountInstantiation(string account, address addr_1, address addr_2);
    
    uint public globalAccountCount;                     // 记录账户数量
    
    mapping(uint => string) accounts;                   // accountId => account
    mapping(string => address[]) accountAddrs;          // account => 账户控制的地址组
    mapping(address => string) addrAccount;             // address => account
    mapping(string => bool) isAccountExists;            // accout => bool
    mapping(address => bool) isAccountAddr;             // address => bool 
  
    
    // 判断账户是否存在
    modifier isAccountExist(string account) {
        require(!isAccountExists[account], "The account has been exist!");
        _;
    }
    
    // 判断地址是否存在
    modifier isAddrExist(address addr_1, address addr_2) {
        require(!isAccountAddr[addr_1] && !isAccountAddr[addr_2], "The addr_1 or addr_2 has been exist!");
        _;
    }    
    
    // 注册
    function register(string account, address addr_1, address addr_2) 
        internal 
        isAccountExist(account)
        isAddrExist(addr_1, addr_2)
    {
        isAccountExists[account] = true;
        isAccountAddr[addr_1] = true;
        isAccountAddr[addr_2] = true;
            
        accounts[++globalAccountCount] = account;
        accountAddrs[account].push(addr_1);    
        accountAddrs[account].push(addr_2);
        
        addrAccount[addr_1] = account;
        addrAccount[addr_2] = account;
            
        emit AccountInstantiation(account, addr_1, addr_2);
    }
    
    // 通过accountId获取账户
    function getAccountsName(uint accountId) public view returns(string) {
        return accounts[accountId];
    }
    
    // 通过account获取账户控制的地址及地址数量
    function getAccountInitInfo(string account)
        public
        view
        returns(string _account, uint addrNum, address accountRoot)
    {
        return (account, accountAddrs[account].length, accountAddrs[account][0]);
    }
    
    // 通过账户和地址ID获取地址
    function getAccountAddrIdInfo(string account, uint addrId)
        public
        view
        returns(uint _addrId, address numAddr)
    {
            return (addrId, accountAddrs[account][addrId]);
    }
    
    //通过地址获取账户
    // function getAddrAccountInfo(address addr) public view returns(string) {
    //     return addrAccount[addr];
    // }
    
    
}