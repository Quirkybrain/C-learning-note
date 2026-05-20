-- 下载 lua 1.1 源代码并在限制内存的情况下
-- 执行此代码，会触发内存溢出

bug_table = @{}

print("开始执行恶意的不断读取操作...")

i = 1
dummy = 0

while 1 do
    
    dummy = bug_table[i]
    if i = 100000 then
        print("已分配 10 万个无用的 nil 节点...")
    elseif i = 500000 then
        print("已分配 50 万个无用的 nil 节点...")
    elseif i = 1000000 then
        print("已分配 100 万个无用的 nil 节点...")
    elseif i = 1500000 then
        print("已分配 150 万个无用的 nil 节点...")
    elseif i = 2000000 then
        print("已分配 200 万个无用的 nil 节点...")
    elseif i = 3000000 then
        print("已分配 300 万个无用的 nil 节点...")
    end
    i = i + 1
end
