Parse by walking the characters: key, `=`, then either a quoted value or a bare value.
===
Compare the whole key length as well as the bytes, and remember the first match only; a value in quotes can contain `=` and spaces without starting a new field.
