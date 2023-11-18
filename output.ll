; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = external global i32
@f = external global float
@b = external global i1

declare i32 @print_int(i32)

define i32 @While(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 0, ptr %result, align 4
  %test = load i32, ptr @test, align 4
  %calltmp = call i32 @print_int(i32 %test)
  br label %cond

cond:                                             ; preds = %body, %entry
  %result2 = load i32, ptr %result, align 4
  %lttmp = icmp slt i32 %result2, 10
  br i1 %lttmp, label %body, label %exitwhile

body:                                             ; preds = %cond
  %result3 = load i32, ptr %result, align 4
  %addtmp = add i32 %result3, 1
  store i32 %addtmp, ptr %result, align 4
  br label %cond

exitwhile:                                        ; preds = %cond
  %result4 = load i32, ptr %result, align 4
  ret i32 %result4
  ret void
}
