struct MyStruct {
    field int;
    field string;
}

fn myfn(a int, b string) void {
    let a *int = 3;
    let b = 4;

    a.b.c = 3;
    a[a * b] = 3;

    if a[a * b] == 3 {
        b = 4;
        if 2 + 2 == 4 {
            b = a;
        };
    } else if 1 < 2 {
        while a < b {
            a = a - b;
        };
    };

    return a[myfn(1, 2, 4, 5) + 2];
}
