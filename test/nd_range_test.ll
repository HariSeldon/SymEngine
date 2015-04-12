target triple = "spir"

define void @testFunction(float addrspace(1)* %odata, float addrspace(1)* %idata, i32 %width, i32 %height) {
entry:
  %call = call i32 @get_global_id(i32 1)
  %call1 = call i32 @get_global_id(i32 0)
  %mul = mul i32 %call, %width
  %add = add i32 %mul, %call1
  %mul2 = mul i32 %call1, %height
  %add3 = add i32 %mul2, %call
  %arrayidx = getelementptr inbounds float addrspace(1)* %idata, i32 %add
  %0 = load float addrspace(1)* %arrayidx, align 4
  %arrayidx4 = getelementptr inbounds float addrspace(1)* %odata, i32 %add3
  store float %0, float addrspace(1)* %arrayidx4, align 4
  ret void
}

declare i32 @get_global_id(i32)

!opencl.kernels = !{!0}

!0 = !{void (float addrspace(1)*, float addrspace(1)*, i32, i32)* @testFunction}
