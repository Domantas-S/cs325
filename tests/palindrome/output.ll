; ModuleID = 'mini-c'
source_filename = "mini-c"

define i1 @palindrome(i32 %number) {
entry:
  %result = alloca i1, align 1
  %rmndr = alloca i32, align 4
  %rev = alloca i32, align 4
  %t = alloca i32, align 4
  %number1 = alloca i32, align 4
  store i32 %number, ptr %number1, align 4
  store i32 0, ptr %rev, align 4
  store i1 false, ptr %result, align 1
  %number2 = load i32, ptr %number1, align 4
  store i32 %number2, ptr %t, align 4
  br label %cond

cond:                                             ; preds = %body, %entry
  %number3 = load i32, ptr %number1, align 4
  %gttmp = icmp sgt i32 %number3, 0
  br i1 %gttmp, label %body, label %exitwhile

body:                                             ; preds = %cond
  %number4 = load i32, ptr %number1, align 4
  %modtmp = srem i32 %number4, 10
  store i32 %modtmp, ptr %rmndr, align 4
  %rev5 = load i32, ptr %rev, align 4
  %multmp = mul i32 %rev5, 10
  %rmndr6 = load i32, ptr %rmndr, align 4
  %addtmp = add i32 %multmp, %rmndr6
  store i32 %addtmp, ptr %rev, align 4
  %number7 = load i32, ptr %number1, align 4
  %divtmp = sdiv i32 %number7, 10
  store i32 %divtmp, ptr %number1, align 4
  br label %cond

exitwhile:                                        ; preds = %cond
  %t8 = load i32, ptr %t, align 4
  %rev9 = load i32, ptr %rev, align 4
  %eqtmp = icmp eq i32 %t8, %rev9
  br i1 %eqtmp, label %then, label %else

then:                                             ; preds = %exitwhile
  store i1 true, ptr %result, align 1
  br label %ifcont

else:                                             ; preds = %exitwhile
  store i1 false, ptr %result, align 1
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %result10 = load i1, ptr %result, align 1
  ret i1 %result10
}
