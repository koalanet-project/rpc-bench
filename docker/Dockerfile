FROM archlinux/base:latest
# archlinux/base image has better locale support

ARG SSH_PUBKEY=sshkey.pub

COPY pacman.conf /etc/
COPY mirrorlist /etc/pacman.d/
COPY locale.gen /etc/

RUN pacman -Syyu yaourt --noconfirm --needed && \
    locale-gen

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV LC_ALL en_US.UTF-8

# base-devel contains binutils, gcc, make, pkgconf, libtool, autoconf, automake...
RUN yaourt -S --noconfirm --needed \
    base base-devel multilib-devel \
    pacman-contrib \
    vim vim-plug \
    zsh grml-zsh-config zsh-syntax-highlighting \
    git \
    gdb clang bazel \
    cmake \
    python python2 python-pip python2-pip \
    openssh \
    tmux screen \
    iperf \
    rsync \
    tree \
    unzip-iconv \
    htop \
    wget curl mtr iputils \
    perf \
    sysstat \
    gnupg \
    openbsd-netcat

COPY vimrc /root/.vimrc

RUN vim -u NONE -S ~/.vimrc +PluginInstall +qall > /dev/null && \
    clang-format -style=google -dump-config | sed 's/ColumnLimit.*/ColumnLimit:     100/g' > ~/.clang-format && \
    chsh -s /usr/bin/zsh root && \
    mkdir /root/.ssh && \
    /usr/bin/ssh-keygen -A

EXPOSE 22

# copy your public keys
COPY ${SSH_PUBKEY} /root/.ssh/authorized_keys

WORKDIR /root

# remember to clean the packages
RUN paccache -r -k 0

CMD ["/usr/bin/zsh"]
