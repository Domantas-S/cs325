; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @addition(i32 %n, i32 %m) {
entry:
  %result = alloca i32, align 4
  %m2 = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 %m, ptr %m2, align 4
  %n3 = load i32, ptr %n1, align 4
  %m4 = load i32, ptr %m2, align 4
  %addtmp = add i32 %n3, %m4
  store i32 %addtmp, ptr %result, align 4
  %n5 = load i32, ptr %n1, align 4
  %eqtmp = icmp eq i32 %n5, 4
  br i1 %eqtmp, label %then, label %end9

then:                                             ; preds = %entry
  %n6 = load i32, ptr %n1, align 4
  %m7 = load i32, ptr %m2, align 4
  %addtmp8 = add i32 %n6, %m7
  %calltmp = call i32 @print_int(i32 %addtmp8)
  br label %ifcont

end9:                                             ; preds = %entry
  %n10 = load i32, ptr %n1, align 4
  %m11 = load i32, ptr %m2, align 4
  %multmp = mul i32 %n10, %m11
  %calltmp12 = call i32 @print_int(i32 %multmp)
  br label %ifcont
}