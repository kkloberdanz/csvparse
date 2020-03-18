FROM centos:6

RUN yum install -y make gcc git zip unzip

WORKDIR /work

RUN mkdir /dist

COPY . /work
