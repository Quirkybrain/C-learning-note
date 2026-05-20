// comment syntax self-check

print("comment-check-start")

modern = 8/2 // modern inline comment should be ignored
legacy = 9/3 -- legacy inline comment should still be ignored
plain = 10/5

if modern = 4 then
 print("modern-ok")
else
 print("modern-bad")
end

if legacy = 3 then
 print("legacy-ok")
else
 print("legacy-bad")
end

if plain = 2 then
 print("division-ok")
else
 print("division-bad")
end

if modern = 4 and legacy = 3 and plain = 2 then
 print("PASS // comments enabled")
else
 print("FAIL // comments not working correctly")
end
