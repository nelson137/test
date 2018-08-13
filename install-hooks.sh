#!/bin/bash

hooks="$(dirname "$0")/.git/hooks"
mkdir -p "$hooks"

rm -f ${hooks}/*.sample



sed -r 's/^ {4}//' > "${hooks}/cpp-compile.sh" <<'EOF'
    #!/bin/bash

    mkdir -p bin/compiled

    # <git dir>/hooks
    hooks="$(dirname "$0")"

    # Which hook this script is being executed by
    context="$1"

    # Hash of last commit on remote
    remote_commit="$2"

    # Hash of last local commit
    current_commit="$3"

    # git diff-tree options
    options='-r --diff-filter=_ --no-commit-id --name-only --relative=bin-src'


    # Added and modified files
    [[ "$context" == post-merge ]] &&
        trees='ORIG_HEAD HEAD'
    [[ "$context" == pre-push ]] &&
        trees="$remote_commit $current_commit"
    to_compile=( $(git diff-tree ${options/_/AM} $trees) )

    # Files not yet compiled
    while read -r file; do
        bin_name="$(basename "${file%.cpp}")"
        [[ ! -f "bin/compiled/$bin_name" ]] &&
            to_compile+=( "$file" )
    done < <(find bin-src -type f -name '*.cpp')

    # Remove duplicate items
    uniq_to_compile=()
    while read -r uniq_file; do
        uniq_to_compile+=( "$uniq_file" )
    done < <(for f in "${to_compile[@]}"; do echo "$f"; done | sort -u)

    # Compile cpp files into bin/compiled/
    for file in "${uniq_to_compile[@]}"; do
        echo "Compiling $file ..."
        bin_name="${file%.cpp}"
        g++ -std=c++11 "bin-src/$file" -o "bin/compiled/$bin_name"
    done

    # Put newline between compile messages and git output
    (( "${#uniq_to_compile[@]}" > 0 )) &&
        echo

    # Delete binaries of removed cpp files
    if [[ "$context" == pre-push ]]; then
        # Removed files (includes old names of renamed files)
        to_rm=(
            $(git diff-tree ${options/_/D} "$remote_commit" "$current_commit")
        )

        # Remove binaries from bin/compiled/
        for file in "${to_rm[@]}"; do
            bin_name="${file%.cpp}"
            echo "Removing bin/compiled/$bin_name ..."
            rm -f "bin/compiled/$bin_name"
        done
    fi
EOF



sed -r 's/^ {4}//' > "${hooks}/post-merge" <<'EOF'
    #!/bin/bash

    # <git dir>/hooks
    hooks="$(dirname "$0")"


    # Compile added, modified, and uncompiled bin-src/ files
    "${hooks}/cpp-compile.sh" post-merge

    # Update plugins if changes were made in plugin section of files/.vimrc
    "${hooks}/update-vim-plugins.sh"
EOF



sed -r 's/^ {4}//' > "${hooks}/pre-push" <<'EOF'
    #!/bin/bash

    # <git dir>/hooks
    hooks="$(dirname "$0")"

    # Url of reciever of the push
    url="$2"

    # Hash of last commit on remote
    remote_commit=$(git ls-remote "$url" | grep HEAD | awk '{print $1}')

    # Hash of last local commit
    current_commit=$(git rev-parse HEAD)


    # Compile added, modified, and uncompiled bin-src/ files
    "${hooks}/cpp-compile.sh" pre-push "$remote_commit" "$current_commit"


    # Update plugins if changes were made in plugin section of files/.vimrc
    "${hooks}/update-vim-plugins.sh"
EOF



sed -r 's/^ {4}//' > "${hooks}/update-vim-plugins.sh" <<'EOF'
    #!/bin/bash
    # Update plugins if plugin changes were made in files/.vimrc

    # <git dir>/hooks
    hooks="$(dirname "$0")"

    # Time of last pull
    last_pull="${hooks}/last-pull"

    # Hash of last local commit
    current="$(git rev-parse HEAD)"


    if [[ -f "$last_pull" ]]; then
        # Find changes to files/.vimrc that involve plugins
        git diff "$(< $last_pull)" "$current" files/.vimrc |
            grep -E '^(\+|-)' | grep -Ev '^(\+|-){3}' |
            grep -E "Plugin (\".+\"|'.+')"

        if [[ $? == 0 ]]; then
            echo 'Updating Vundle plugins ...'
            vim +PluginClean! +PluginInstall +qall 2>/dev/null
            echo
        fi
    fi

    echo "$current" > "$last_pull"
EOF



chmod +x "${hooks}/"{cpp-compile.sh,post-merge,pre-push,update-vim-plugins.sh}
