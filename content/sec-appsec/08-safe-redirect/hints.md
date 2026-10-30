Write the checks as a loop over every byte, with the special rules for the second character afterwards.
===
For `%`, decode the two hex digits to a number and compare the number, so upper and lower case digits behave the same.
