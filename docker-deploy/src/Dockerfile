FROM ubuntu:20.04
RUN apt-get update && \
    apt-get install -y make && \
    apt-get install -y libboost-all-dev && \
    apt-get install -y g++ && \
    apt-get install -y clean
RUN mkdir /var/log/erss
RUN touch /var/log/erss/proxy.log
RUN mkdir /code
ADD . /code
WORKDIR /code
RUN ["chmod", "+x", "start.sh"]
EXPOSE 12345