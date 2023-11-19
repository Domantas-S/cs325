; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define i32 @foo(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, ptr %x1, align 4
  br label %cond

cond:                                             ; preds = %ifcont, %entry
  %x2 = load i32, ptr %x1, align 4
  %lttmp = icmp slt i32 %x2, 10
  br i1 %lttmp, label %body, label %exitwhile

body:                                             ; preds = %cond
  %x3 = load i32, ptr %x1, align 4
  %lttmp4 = icmp slt i32 %x3, 5
  br i1 %lttmp4, label %then, label %else

exitwhile:                                        ; preds = %cond
  %x8 = load i32, ptr %x1, align 4
  ret i32 %x8

then:                                             ; preds = %body
  %x5 = load i32, ptr %x1, align 4
  %addtmp = add i32 %x5, 2
  store i32 %addtmp, ptr %x1, align 4
  br label %ifcont

else:                                             ; preds = %body
  %x6 = load i32, ptr %x1, align 4
  %addtmp7 = add i32 %x6, 1
  store i32 %addtmp7, ptr %x1, align 4
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  br label %cond
}
