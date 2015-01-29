<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <html>
      <head>
        <title>PhoenixTibia Plugins</title>
      </head>
      <body>
        <h2>Plugins</h2>
        <h4>Enabled Plugins</h4>
        <ul>
          <xsl:apply-templates select="plugins/plugin[@enabled='yes']">
            <xsl:sort select="@id"/>
          </xsl:apply-templates>
        </ul>

        <h4>Disabled Plugins</h4>
        <ul>
          <xsl:apply-templates select="plugins/plugin[@enabled='no']">
            <xsl:sort select="@id"/>
          </xsl:apply-templates>
        </ul>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="plugin">
    <li>
      <xsl:value-of select="@id"/>: <xsl:value-of select="@name"/>
      <xsl:if test="option">
        <br/>
        Options:
        <ul>
          <xsl:apply-templates select="option">
            <xsl:sort select="@name"/>
          </xsl:apply-templates>
        </ul>
      </xsl:if>
    </li>
  </xsl:template>

  <xsl:template match="option">
    <li>
      Name: <xsl:value-of select="@name"/><br/>
      Value: <xsl:value-of select="@value"/>
    </li>
  </xsl:template>
</xsl:stylesheet>
