#!/usr/bin/env bash
set -e

# Run this script from the repository root to import Proxmark3 crapto1 into
# the gpl-crypto1-import branch using git subtree (squashed commit).
# After running it, push the branch to origin and open a PR to main.

REPO_URL="https://github.com/Proxmark/proxmark3"
REMOTE_NAME="proxmark"
PREFIX="esp32/MifareBreaker/crypto/crapto1"
BRANCH="master"

echo "This will import upstream ${REPO_URL} branch ${BRANCH} into ${PREFIX} as a squashed subtree commit."
read -p "Are you sure? (y/N) " yn
if [[ "$yn" != "y" && "$yn" != "Y" ]]; then
  echo "Aborting."
  exit 1
fi

# Add remote if not present
if ! git remote get-url ${REMOTE_NAME} >/dev/null 2>&1; then
  git remote add ${REMOTE_NAME} ${REPO_URL}
fi

git fetch ${REMOTE_NAME}

echo "Adding subtree..."
git subtree add --prefix=${PREFIX} ${REMOTE_NAME} ${BRANCH} --squash

echo "Subtree import complete. Review the changes, then:"
echo "  git push origin gpl-crypto1-import"
echo "Open a PR from gpl-crypto1-import to main and ensure owner explicitly accepts GPL relicensing implications before merging."
