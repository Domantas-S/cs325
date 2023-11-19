; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define i32 @unary2() {
entry:
  %ortmp = alloca i1, align 1
  %X = alloca i1, align 1
  %z2 = alloca i32, align 4
  %z = alloca i32, align 4
  %b2 = alloca i1, align 1
  %b = alloca i1, align 1
  %i2 = alloca i32, align 4
  %i = alloca i32, align 4
  %f = alloca float, align 4
  store i1 true, ptr %X, align 1
  store i32 0, ptr %i2, align 4
  store float 0.000000e+00, ptr %f, align 4
  store i32 1, ptr %i, align 4
  %f1 = load float, ptr %f, align 4
  %FPtoBcast = fptosi float %f1 to i1
  %nottmp = xor i1 %FPtoBcast, true
  %negtmp = sub i1 false, %nottmp
  %i3 = load i32, ptr %i, align 4
  %BtoSIcast = zext i1 %negtmp to i32
  %addtmp = add i32 %BtoSIcast, %i3
  %SItoBcast = trunc i32 %addtmp to i1
  store i1 %SItoBcast, ptr %b, align 1
  %f4 = load float, ptr %f, align 4
  %FPtoBcast5 = fptosi float %f4 to i1
  %nottmp6 = xor i1 %FPtoBcast5, true
  %negtmp7 = sub i1 false, %nottmp6
  %BtoSIcast8 = zext i1 %negtmp7 to i32
  store i32 %BtoSIcast8, ptr %z2, align 4
  br label %lhs

lhs:                                              ; preds = %entry
  br i1 true, label %settrue, label %rhs

rhs:                                              ; preds = %lhs
  %b9 = load i1, ptr %b, align 1
  br i1 %b9, label %settrue, label %setfalse

end:                                              ; preds = %setfalse, %settrue
  %ortmp10 = load i1, ptr %ortmp, align 1
  store i1 %ortmp10, ptr %b2, align 1
  %b211 = load i1, ptr %b2, align 1
  %BtoSIcast12 = zext i1 %b211 to i32
  store i32 %BtoSIcast12, ptr %i2, align 4
  %i213 = load i32, ptr %i2, align 4
  ret i32 %i213

settrue:                                          ; preds = %rhs, %lhs
  store i1 true, ptr %ortmp, align 1
  br label %end

setfalse:                                         ; preds = %rhs
  store i1 false, ptr %ortmp, align 1
  br label %end
}
