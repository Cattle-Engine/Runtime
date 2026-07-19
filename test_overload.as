void test(int x) {
    // int overload
}

void test(string s) {
    // string overload
}

void test(float f, string@ s) {
    // float & string handle overload
}

void main() {
    test(10);
    test("hello");
    test(3.14f, null);
}
