program pt(input, output, stdErr);
uses crt;
const tata = 4; titi = 123; pi = 3.14;
var x, y, a: integer; test: string = 'Hi'; test2: real = 1; { Non-initialized global variables should be given a zero value + warning to user }

procedure findMin(x, y, z: integer; var m: integer);
{ Finds the minimum of the 3 values and stocks it in a variable m }
begin
    if x < y then
        m:= x
    else
        m:= y;
    if z < m then
        m:= z;
end; { end of procedure findMin }

function findMax(num1, num2: integer): integer;
{ Finds and returns the maximum of 2 values }
var result: integer;
begin
    writeln(result); { Local variables have a random value }
    if (num1 > num2) then
        result := num1
    else
        result := num2;
    findMax := result;
end; { end of function findMax }

function findMax(num1, num2: integer; num3: real): integer;
{ Finds and returns the maximum of 2 values }
var result: integer;
begin
    if (num1 > num2) then
        result := num1
    else
        result := num2;
    findMax := result;
end; { end of function findMax }

begin
    a := x + y;
    x := tata + titi;
    y := titi;
    if (x <= 9) and (y >= 100) then
    begin
        y := 10 - x;
        x := x + y
    end
    else
        writeln('Hey');
    test := 'Hello World !';
    writeln(x + y);
    writeln(test);
    for a := (1+0) to tata do write(a);
    while a > 1 do
    begin
        a := a - 1;
        write(a);
    end;
    findMin(2, 3, 4, x);
    x := findMax(1, x);
end.