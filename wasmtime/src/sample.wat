(module
  (memory 1 2)
  (func (export "add") (param i32 i32) (result i32)
    i32.const 100
    i64.const 0
    i64.store

    i32.const 0
    memory.grow
    drop

    i32.const 1
    memory.grow
    drop


    local.get 0
    local.get 1
    i32.add))
