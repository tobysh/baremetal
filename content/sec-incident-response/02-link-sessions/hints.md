Handle the session case completely before looking at addresses; it returns early in every branch.
===
Use a set of users for the address case so one user logging in twice is not "ambiguous".
===
A login with a matching session but a later time than the request does not count.
