; ModuleID = 'mini-c'
source_filename = "mini-c"

define i32 @returns(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  br label %cond

cond:                                             ; preds = %body, %entry
  %x2 = load i32, ptr %x1, align 4
  %eqtmp = icmp eq i32 %x2, 1
  br i1 %eqtmp, label %body, label %exitwhile

body:                                             ; preds = %cond
  ret i32 0
  br label %cond

exitwhile:                                        ; preds = %cond
  %x3 = load i32, ptr %x1, align 4
  %gttmp = icmp sgt i32 %x3, 1
  br i1 %gttmp, label %then, label %else

then:                                             ; preds = %exitwhile
  ret i32 1
  br label %ifcont

else:                                             ; preds = %exitwhile
  ret i32 2
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  ret i32 3
  ret i32 0
}
