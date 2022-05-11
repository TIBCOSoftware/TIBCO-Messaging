import pathlib
import setuptools
from setuptools import setup

# The directory containing this file
HERE = pathlib.Path(__file__).parent

# The text of the README file
README = (HERE / "README.md").read_text(encoding='utf-8')

# This call to setup() does all the work
setup(
    name="eftl",
    version="1.2.0",
    description="TIBCO eFTL client for Python",
    long_description=README,
    long_description_content_type="text/markdown",
    author="TIBCO Software Inc.",
    author_email="support@tibco.com", 
    url="https://www.tibco.com",
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
    ],
    packages=setuptools.find_packages(),
    include_package_data=True,
    install_requires=["asyncio", "datetime", "jsonpickle", "autobahn", "pyOpenSSL", "service_identity"],
    entry_points={},
)
