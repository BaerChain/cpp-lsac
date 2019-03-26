pragma solidity ^0.4.25;

contract TokenBRC {
    
    // 事件
    event Transfer(string from, string to, uint value);
    
    // 存储账户余额
    mapping (string => uint) balances;
    mapping (address => mapping (string => uint)) _allowed;
    
    string private name;
    
    constructor() public {
        name = "brctoken";
        balances[name] = 10000 wei;
    }
     
    // 获取指定账户的余额
    function balanceof(string account) public view returns(uint) {
        return balances[account];
    }    
    
    // 账户账户转账到其它指定账户
    function transferFrom(string from, string to, uint value) public returns(bool) {
        _transfer(from, to, value);
        return true;
    }
    
    function _transfer(string from, string to, uint value) internal {
        require(value > 0 
            && balances[from] >= value 
            && balances[to] + value > balances[to], 
            "The from should have more balances or the value is invalid!");
        
        balances[from] = balances[from] - value;
        balances[to] = balances[to] + value;
        
        emit Transfer(from, to, value);
    }
    
    // 超级地址可以批准其它地址使用的金额
    function allowance(address addrRoot, address spender) public view returns (uint256) {
        return _allowed[addrRoot][spender];
    }
    
}








