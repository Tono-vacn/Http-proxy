FROM ubuntu:20.04
RUN apt-get update && apt-get install -y make
RUN apt-get install -y make
RUN apt-get install -y libboost-all-dev
RUN mkdir /var/log/erss
RUN touch /var/log/erss/proxy.log
RUN mkdir /code
ADD . /code
WORKDIR /code
RUN ["chmod", "+x", "start.sh"]
