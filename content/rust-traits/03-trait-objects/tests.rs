#[test]
fn dog_and_cat_identities() {
    assert_eq!(Dog.name(), "Dog", "Dog name");
    assert_eq!(Dog.sound(), "woof", "Dog sound");
    assert_eq!(Cat.name(), "Cat", "Cat name");
    assert_eq!(Cat.sound(), "meow", "Cat sound");
}

#[test]
fn generic_speak() {
    assert_eq!(speak_generic(&Dog), "Dog says woof", "speak_generic for Dog");
    assert_eq!(speak_generic(&Cat), "Cat says meow", "speak_generic for Cat");
}

#[test]
fn dyn_speak() {
    assert_eq!(speak_dyn(&Dog), "Dog says woof", "speak_dyn for Dog");
    let boxed: Box<dyn Animal> = Box::new(Cat);
    assert_eq!(speak_dyn(boxed.as_ref()), "Cat says meow", "speak_dyn for a boxed Cat");
}

#[test]
fn chorus_mixed_animals_in_order() {
    let zoo: Vec<Box<dyn Animal>> = vec![Box::new(Cat), Box::new(Dog), Box::new(Cat)];
    assert_eq!(
        chorus(&zoo),
        "Cat says meow; Dog says woof; Cat says meow",
        "chorus should join in order with '; '"
    );
}

#[test]
fn chorus_single_has_no_separator() {
    let zoo: Vec<Box<dyn Animal>> = vec![Box::new(Dog)];
    assert_eq!(chorus(&zoo), "Dog says woof", "one animal, no separator");
}

#[test]
fn chorus_empty_is_empty() {
    let zoo: Vec<Box<dyn Animal>> = Vec::new();
    assert_eq!(chorus(&zoo), "", "no animals gives an empty string");
}
