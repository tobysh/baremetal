Last updated: {{lastUpdated}}

This page explains the cookies and browser storage Baremetal uses, why we use them, and how you can change your choices. Cookies are small text files that a website saves in your browser. For how we use your data more generally, see our [Privacy Policy](/privacy).

## The short answer

We only use cookies that the site needs to work. We do not use analytics cookies. Analytics, which you can choose to turn on, uses no cookies and no local storage.

## Cookies we use

| Name | Purpose | Type | Duration |
|---|---|---|---|
| __Host-bm_session | Keeps you signed in | Strictly necessary | Up to 30 days, or 12 hours for administrators; ends sooner if unused for 14 days (2 hours for administrators) |
| bm_consent | Remembers your cookie choice | Strictly necessary | 6 months |
| __Host-bm_2fa_pending | Marks that you are part way through signing in with two-factor authentication | Strictly necessary | 10 minutes |

Strictly necessary cookies are needed for the site to work, so we do not ask for consent to set them.

The sign-in cookie, __Host-bm_session, is set in a way that keeps it safer:

- It cannot be read by scripts on the page (HttpOnly).
- It is only sent over encrypted connections (Secure). The one exception is a local development copy served over plain HTTP, where an operator can switch the flag off so sign-in works on localhost. Where Secure cookies are on, browsers also accept it only with the __Host- name prefix, which ties it to this exact site.
- Your browser will not send it with requests that start on other websites (SameSite=Strict).

## Browser storage

When you're not signed in, the code you write in a lesson is saved in your browser's local storage so you don't lose it if you reload the page. This is not a cookie, it is never sent to us automatically, and it stays in your browser on that device. You can remove it by clearing your browser's site data.

When you are signed in, we keep a browser recovery copy of an edited lesson draft until the latest account save succeeds. Recovery copies are separated by account and exercise and can be removed by clearing this site’s browser storage. Review drafts do not overwrite your saved solution.

We also save your light or dark theme choice in your browser's local storage, under the name bm:theme. It stores only "light" or "dark". It is a functional preference that the site needs to show the look you picked, so we do not ask for consent. It is never sent to our servers and stays in your browser on that device. You can remove it by clearing your browser's site data, and the site will go back to your device's light or dark setting.

The code you write in a lesson is kept under bm:code: followed by the lesson ID when you are signed out, and the signed-in recovery copy described above is kept under bm:draft: followed by your account ID and the lesson ID. These hold your own code text, are never sent to us automatically, and are replaced or removed once a save succeeds.

The workspace remembers the reading pane width you choose under bm:workspace:reading-width. This stores only a width percentage between 32 and 60. In the same way, bm:workspace:editor-split stores the editor height you choose as a percentage between 30 and 75. The editor remembers your text size under bm:editor:font-size (a number from 12 to 24) and your indentation width under bm:editor:tab-width (2, 4 or 8) and whether long lines wrap under bm:editor:line-wrap (1 when on). If browser storage is blocked, these controls still work for the current page session. Once you finish or skip the short workspace tour, bm:workspace:tour-done stores "1" so it is not shown again. If you dismiss the dashboard's username reminder, bm:username-notice: followed by your account ID stores "dismissed", so that reminder stays hidden for your account in that browser. These functional preferences are not sent to our servers and remain on that device until you clear the site's browser storage. Clearing them restores the default workspace width and editor height and allows the reminder to appear again.

### Service worker cache

Baremetal installs a small service worker (/sw.js) that keeps a cache named bm-v1-static in your browser. It stores only the built script and style files the site ships (under /_next/static/), the app icons and the offline page, so repeat visits load faster and the offline page can show when you have no connection. It never stores pages, lesson content, account data or API responses. You can remove it by clearing this site's data in your browser, or by unregistering the service worker in your browser's developer settings.

## Analytics

If you say yes, our analytics records page visits and some device details. It uses a visitor ID that changes every day. It sets no cookies and uses no local storage. See the analytics section of our [Privacy Policy](/privacy) for the full details.

## Your choices

- You can change your cookie choice at any time. Use the "Cookie settings" link in the footer, or go to [Settings](/settings).
- You can clear cookies in your browser settings. Each browser has its own steps. Look for "cookies" or "site data".
- If you clear bm_consent, your choice is forgotten, so you may see the cookie banner again.
- If you block __Host-bm_session, you will not be able to stay signed in.

## Changes to this page

We may update this page if we change the cookies or storage we use.

## Contact

{{entity}}, {{address}}, {{contactEmail}}
