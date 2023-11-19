; ModuleID = 'mini-c'
source_filename = "mini-c"

@mutable_var = common global i32 0

define i32 @mutating_function() {
entry:
  %mutable_var = load i32, ptr @mutable_var, align 4
  %addtmp = add i32 %mutable_var, 1
  store i32 %addtmp, ptr @mutable_var, align 4
  ret i32 1
}

define i32 @lazyeval_and(i32 %control) {
entry:
  %andtmp = alloca i1, align 1
  %control1 = alloca i32, align 4
  store i32 %control, ptr %control1, align 4
  store i32 0, ptr @mutable_var, align 4
  br label %lhs

lhs:                                              ; preds = %entry
  %control2 = load i32, ptr %control1, align 4
  %eqtmp = icmp eq i32 %control2, 1
  br i1 %eqtmp, label %rhs, label %setfalse

rhs:                                              ; preds = %lhs
  %calltmp = call i32 @mutating_function()
  %SItoBcast = trunc i32 %calltmp to i1
  br i1 %SItoBcast, label %settrue, label %setfalse

end:                                              ; preds = %setfalse, %settrue
  %andtmp3 = load i1, ptr %andtmp, align 1
  br i1 %andtmp3, label %then, label %else

setfalse:                                         ; preds = %rhs, %lhs
  store i1 false, ptr %andtmp, align 1
  br label %end

settrue:                                          ; preds = %rhs
  store i1 true, ptr %andtmp, align 1
  br label %end

then:                                             ; preds = %end
  %mutable_var = load i32, ptr @mutable_var, align 4
  ret i32 %mutable_var
  br label %ifcont

else:                                             ; preds = %end
  %mutable_var4 = load i32, ptr @mutable_var, align 4
  ret i32 %mutable_var4
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  ret i32 0
}

define i32 @lazyeval_or(i32 %control) {
entry:
  %ortmp = alloca i1, align 1
  %control1 = alloca i32, align 4
  store i32 %control, ptr %control1, align 4
  store i32 0, ptr @mutable_var, align 4
  br label %lhs

lhs:                                              ; preds = %entry
  %control2 = load i32, ptr %control1, align 4
  %eqtmp = icmp eq i32 %control2, 1
  br i1 %eqtmp, label %settrue, label %rhs

rhs:                                              ; preds = %lhs
  %calltmp = call i32 @mutating_function()
  %SItoBcast = trunc i32 %calltmp to i1
  br i1 %SItoBcast, label %settrue, label %setfalse

end:                                              ; preds = %setfalse, %settrue
  %ortmp3 = load i1, ptr %ortmp, align 1
  br i1 %ortmp3, label %then, label %else

settrue:                                          ; preds = %rhs, %lhs
  store i1 true, ptr %ortmp, align 1
  br label %end

setfalse:                                         ; preds = %rhs
  store i1 false, ptr %ortmp, align 1
  br label %end

then:                                             ; preds = %end
  %mutable_var = load i32, ptr @mutable_var, align 4
  ret i32 %mutable_var
  br label %ifcont

else:                                             ; preds = %end
  %mutable_var4 = load i32, ptr @mutable_var, align 4
  ret i32 %mutable_var4
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  ret i32 0
}
