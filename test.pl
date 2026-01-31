struct MyStruct {
    field int;
    field string;
}

fn myfn(a int, b string) void {
    1 + 1;
    a = 3;
    a.b.c = 3;
    a[1] = 3;
    return a[myfn(1, 2, 4, 5) + 2];
}
