package main

func BrowserAnswers() map[string]string {
	return map[string]string{"csrf-missing": "reject", "cors-side-effect": "possible", "cors-wildcard": "blocked", "cors-explicit": "readable", "login-session": "rotate", "httponly": "no", "logout": "invalidate"}
}

func main() {}
