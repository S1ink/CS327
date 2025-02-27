// no exception handling in C
// -->
// ex. "growing if's for handling failures"

// do something
goto a; // on fail

// do something
goto b; // on fail

// do something
goto c; // on fail

c:
// deinit c
b:
// deinit b
a:
// deinit a

exit();