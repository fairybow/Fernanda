Fernanda Privacy Statement

Fernanda keeps your writing on your computer. On its own, it never sends your vaults, your files, or anything you write.

The one thing it sends on its own is the update check. Unless it's turned off, Fernanda asks GitHub for its list of releases a few seconds after it starts and once a day while it runs. Each check is a single request to api.github.com. Like any web request, it shows GitHub your IP address, and it names Fernanda's version. GitHub's privacy statement covers what GitHub does with that:

https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement

To turn the check off, uncheck "Check for updates automatically" when installing on Windows, or switch it off in Settings, under General. The Check for updates button there still checks when you click it.

If a vault is in a Git repository, Fernanda's Git page can fetch, pull, push, and publish a branch, using the Git installed on your computer. Reading the repository's changes and branches stays on your computer. Talking to the remote happens only when you click the Git page's sync button, never on its own, and only with the remote the repository is already set up with. A push or a publish sends your commits to that remote, as Git does from a terminal; what the remote's host does with them is covered by its own terms. Signing in is left to Git and whatever credential manager or SSH agent it uses: Fernanda never sees or keeps a password or token. To turn the Git page off for a vault, switch off Show Git in that vault's Settings, under Git.

Fernanda opens web pages in your browser only when you click a link or a button that says it does, such as Open release page or Create pull request.
