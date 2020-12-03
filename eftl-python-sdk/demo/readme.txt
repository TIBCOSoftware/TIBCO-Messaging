/*
 * @COPYRIGHT_BANNER@
 */

Requirements:
  Python 3.x
    yum install python3 (this will automatically install pip3)
    apt install python3
  pip
    apt install python3-pip

1. Build the python client package.
  1a. egg format
    python setup.py bdist_egg
    easy_install dist/eftl-@TIB_VERSION_FULL@-py3.6.egg
  1b. wheel format
    python set.py bdist_wheel
    pip install dist/eftl-@TIB_VERSION_FULL@-py3.6.whl
  1c. install build files
    python setup.py install

2. Ensure that a FTLServer is running with an eFTL service configured prior to
running the demo.

3. Set the environment variables TIBCO_TEST_URL and TIBCO_TEST_PASSWD.
    To use the 'twisted' library, set the environment variable
TIBCO_TEST_TWISTED.

4. Run demo_receive.py

5. Run demo_send.py
    If you only want to send one message, you may type it with or without
quotes as a command line argument.

6. These demos run indefinitely, so issue a keyboard interrupt to stop.
