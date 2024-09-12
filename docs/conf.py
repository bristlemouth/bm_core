# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'bristlemouth'
author = 'Bristlemouth'
release = '0.0.1'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['myst_parser']

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_tile = 'Bristlemouth Integration'
html_theme = 'sphinx_rtd_theme'
html_static_path = ['']
html_show_sourcelink = False
html_show_copyright = False
html_show_sphinx = False
html_theme_options = {
    'analytics_anonymize_ip': False,
    'logo_only': True,
    'display_version': True,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'collapse_navigation': True,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False,
}
html_logo = 'https://global.discourse-cdn.com/standard17/uploads/bristlemouth/original/1X/7f7c25cb6f75217f8eb21508bb2a2229d2adbfbb.png'
