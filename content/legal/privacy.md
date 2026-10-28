Last updated: {{lastUpdated}}

This policy explains what personal data {{entity}} collects when you use Baremetal, why we collect it, how long we keep it, and what rights you have. We have written it in plain English on purpose. Please also read our [Terms of Service](/terms) and [Cookie Policy](/cookies).

## Who we are

{{entity}} runs Baremetal, the site where you learn C and Rust. For the personal data described here, we are the controller. That means we decide why and how it is used.

- Address: {{address}}
- Contact: {{contactEmail}}

## Who can use Baremetal

You must be 16 or older to use Baremetal. If you are younger, you can only use it where local law allows it and a parent or guardian has given consent.

## The short version

- We only collect what we need to run the course, keep accounts safe, and, if you say yes, understand how the site is used.
- We do not sell your data, use advertising, or profile you.
- {{tutorShort}} Do not paste personal information into the tutor or into your code.
- You can export or delete your data yourself in [Settings](/settings).

## What we collect, why, and for how long

We rely on one of three legal reasons, called lawful bases, for each kind of data:

- **Contract**: we need the data to give you the service you signed up for.
- **Legitimate interest**: we need the data to keep Baremetal and accounts secure. We have weighed this against your rights.
- **Consent**: you choose whether to allow it, and you can take that choice back at any time.

| Data | Why we use it | Lawful basis | How long we keep it |
|---|---|---|---|
| Account: name, email, password, experience level, date and version of Terms you accepted | To give you an account | Contract | Until you delete your account |
| Two-factor authentication (optional, required for admins): authenticator secret and backup codes | To keep your account safe when you sign in | Contract; legitimate interest in security | Until you turn it off or delete your account |
| Sessions: a session token and its creation and expiry times | To keep you signed in | Contract | 30 days, then automatically deleted |
| Learning progress: code you write in exercises, whether you passed, number of attempts, how many hints you opened, whether you used the tutor before passing (to award the Unaided seal), timestamps | To track your progress through the course | Contract | Until you delete your account |
| XP history, achievements and review schedule: when each XP award, achievement and review happened | To show your level, your achievements and the lessons due for review | Contract | Until you delete your account |
| Learning rhythm: your weekly lesson goal and the time zone your browser reports | To work out your weeks (Monday to Sunday) and your weekly rhythm. You can change both in [Settings](/settings) | Contract | Until you delete your account |
| Public profile: username, level, XP, completed lessons, seals, achievements and your best weekly rhythm | To show your progress on the [leaderboard](/leaderboard) and your public profile page (/u/your-username). Visible to anyone unless you hide your profile with the toggle in [Settings](/settings) | Contract | Until you delete your account or hide your profile |
| AI tutor conversations | To answer your questions | Contract | Until you clear them or delete your account |
| Code you run | To compile and run your code | Contract | Deleted straight after the run (only code saved as progress is kept) |
| Security logs | To spot and stop attacks on accounts | Legitimate interest | 90 days |
| Rate limiting counters | To slow down abuse | Legitimate interest | Minutes, and never more than 1 hour |
| Password breach check | To check new passwords against known data breaches | Legitimate interest | See the password section below |
| Password reset emails | To send you a reset link | Contract | Each reset link expires after 30 minutes and works only once |
| Analytics (only if you say yes) | To understand how pages are used | Consent | 180 days |

## How the main parts work

### AI tutor

Your messages and the tutor's replies are stored in your account. When you ask the tutor something, the current lesson, your current code, and the output from your last run are used to write the reply.

{{tutorProcessing}}

To keep the tutor fast and its costs down, when the first question in a conversation exactly matches an earlier one (same lesson, same question, same code and same output), we may reuse the earlier reply instead of generating a new one. These saved replies are stored under a one-way fingerprint, are not linked to your account, and are deleted after 30 days. We also log how many words ("tokens") each tutor reply used, with the lesson and time but not your identity, for 180 days, so we can manage costs.

Do not paste personal information into the tutor or your code. Tutor replies can also be wrong, so read our [Terms of Service](/terms) before you rely on them.

### Running code

Code you run is compiled and executed in an isolated sandbox on our server. The sandbox is deleted straight after the run. The only code we keep is the code you save as progress.

### Passwords and the breach check

We store your password only as a salted scrypt hash. We never store it in readable form.

When you choose a password, we check it against the Have I Been Pwned "Pwned Passwords" service. We use k-anonymity for this. We send only the first 5 characters of a SHA-1 hash of the password. Your password and its full hash never leave our server.

### Public profile

If you choose a username, Baremetal shows your username, level, XP, completed lessons, the seals you have earned, your achievements and your best weekly rhythm on the [leaderboard](/leaderboard) and on your profile page at /u/your-username. Anyone can see these pages, including people who are not signed in. We do not show your name, email or the code you write. You can hide your profile at any time with the profile toggle in [Settings](/settings). When it is hidden, your profile page is not available and you no longer appear on the leaderboard.

### Security logs and rate limits

Security logs record failed sign-in attempts, lockouts, password resets, and two-factor events. Each entry includes your IP address, the email address entered, your browser's user agent, and the time. We use these logs to protect accounts from attacks.

Rate limits count requests by IP address or account for a few minutes. This slows down people who try to abuse the site.

### Analytics (only with consent)

Analytics is off unless you say yes in the cookie banner. You can change your choice at any time in [Settings](/settings) or with the "Cookie settings" link in the footer.

If you say yes, we record:

- the page path you visit
- the domain of the website that referred you, if there is one
- your device type (mobile or desktop)
- a visitor ID

The visitor ID is a one-way hash of your IP address, your browser user agent, and a secret salt. The salt changes every day. Because of this, we cannot follow a visitor from one day to the next, and we never store your raw IP address for analytics.

Analytics is first-party and runs on our own servers. We do not use third-party analytics or advertising, and we do not sell data or profile you. Analytics does not use cookies or local storage. See our [Cookie Policy](/cookies) for more.

## Who we share data with

{{tutorSharing}}- **Our email delivery provider**: sends password reset emails to your address.
- **Have I Been Pwned**: receives only the first 5 characters of a password hash, as described above. It never receives your password.

We do not sell your data.

## Your rights

Under the UK GDPR and the EU GDPR, you have the right to:

- access the personal data we hold about you
- correct data that is wrong
- have your data erased
- restrict how we use your data
- get your data in a portable format
- object to processing based on legitimate interest
- withdraw your consent at any time, for example for analytics
- not be subject to decisions made only by automated means. We do not make any decisions like this.
- complain to a supervisory authority. In the UK this is the Information Commissioner's Office at [ico.org.uk](https://ico.org.uk). In the EU, you can complain to your local authority.

### Do it yourself

In [Settings](/settings), you can export all of your data as JSON. You can also delete your account there.

### Ask us

For any other request, email {{contactEmail}}. We will answer within one month.

## Keeping your data safe

- Data travels over encrypted connections. We use HTTPS in production.
- Passwords and session tokens are stored as hashes.
- Code runs in a sandbox.
- Two-factor authentication is available, and it is required for admins.
- Rate limiting slows down attacks.

No online service is perfectly safe. Please use a strong, unique password and turn on two-factor authentication.

## Changes to this policy

We may update this policy. If a change is important, we will tell you by email or in the app.

## Contact

{{entity}}, {{address}}, {{contactEmail}}
