# -*- coding: utf-8 -*-
import logging

LOGLEVEL = {    'NOTSET': logging.NOTSET,   'DEBUG'     : logging.DEBUG,    \
                'INFO'  : logging.INFO,     'WARNING'   : logging.WARNING,  \
                'ERROR' : logging.ERROR,    'CRITICAL'  : logging.CRITICAL }


class Logger(object):
    def __init__(self, path = None, clevel = logging.INFO, Flevel = logging.INFO):
        self.logger = logging.Logger(__name__)
        # self.logger = logging.getLogger(__name__)
        # self.logger.setLevel(clevel)
        if Flevel == logging.DEBUG or clevel == logging.DEBUG:
            formatter = logging.Formatter('[%(module)s]: (%(lineno)d) (%(asctime)s) %(message)s')
        else:
            formatter = logging.Formatter('(%(asctime)s) %(message)s')

        #设置CMD日志
        sh = logging.StreamHandler()
        sh.setFormatter(formatter)
        sh.setLevel(clevel)
        self.logger.addHandler(sh)

        #设置文件日志
        if path:
            fh = logging.FileHandler(path)
            fh.setFormatter(formatter)
            fh.setLevel(Flevel)
            self.logger.addHandler(fh)

        self.debug      = self.logger.debug
        self.info       = self.logger.info
        self.warning    = self.logger.warning
        self.error      = self.logger.error
        self.critical   = self.logger.critical



if __name__ == "__main__":
    LOG = Logger('log.txt')
    # LOG = Logger()

    LOG.debug('logger debug message')
    LOG.info('logger info message')
    LOG.warning('logger warning message')
    LOG.error('logger error message')
    LOG.critical('logger critical message')



