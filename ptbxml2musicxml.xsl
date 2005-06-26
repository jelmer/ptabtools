<?xml version='1.0'?>
<!--
	PowerTab to MusicXML converter. 
	(C) 2004-2005 Jelmer Vernooij <jelmer@samba.org>
	Published under the GNU GPL
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:exsl="http://exslt.org/common"
	version="1.1">

	<xsl:output indent="yes" method="xml" encoding="UTF-8" doctype-public="-//Recordare//DTD MusicXML 1.0 Partwise//EN" doctype-system="http://www.musicxml.org/dtds/partwise.dtd"/>

	<xsl:template match="powertab">
		<xsl:element name="score-partwise">
			<xsl:call-template name="identification"/>
			<xsl:call-template name="part-list"/>
			<xsl:for-each select="instrument">
				<xsl:call-template name="part"/>
			</xsl:for-each>
		</xsl:element>
	</xsl:template>

	<xsl:template name="part">
		<xsl:element name="part">
			<xsl:attribute name="id">
				<xsl:text>P</xsl:text>
				<xsl:value-of select="@id"/>
			</xsl:attribute>
			<xsl:apply-templates select="sections"/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="words-by">
		<xsl:element name="creator">
			<xsl:attribute name="type">
				<xsl:text>words-by</xsl:text>
			</xsl:attribute>
			<xsl:apply-templates/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="music-by">
		<xsl:element name="creator">
			<xsl:attribute name="type">
				<xsl:text>music-by</xsl:text>
			</xsl:attribute>
			<xsl:apply-templates/>
		</xsl:element>
	</xsl:template>


	<xsl:template match="copyright">
		<xsl:element name="rights">
			<xsl:apply-templates/>
		</xsl:element>
	</xsl:template>

	<xsl:template name="part-list">
		<xsl:element name="part-list">
			<xsl:for-each select="instrument">
				<xsl:element name="score-part">
					<xsl:attribute name="id"><xsl:text>P</xsl:text><xsl:value-of select="@id"/></xsl:attribute>
					<xsl:element name="part-name">
						<xsl:choose>
							<xsl:when test="@id=0">
								<xsl:text>Guitar</xsl:text>
							</xsl:when>
							<xsl:when test="@id=1">
								<xsl:text>Bass</xsl:text>
							</xsl:when>
						</xsl:choose>
					</xsl:element>
				</xsl:element>
			</xsl:for-each>
		</xsl:element>
	</xsl:template>
	
	<xsl:template name="identification">
		<xsl:element name="identification">
			<xsl:for-each select="header/*">
				<xsl:apply-templates/>
			</xsl:for-each>
			<xsl:element name="encoding">
				<xsl:for-each select="header/song/guitar-transcribed-by|header/song/bass-transcribed-by">
					<xsl:element name="encoder">
						<xsl:apply-templates/>
					</xsl:element>
				</xsl:for-each>
				<xsl:element name="software">
					<xsl:text>ptabtools</xsl:text>
				</xsl:element>
			</xsl:element>
		</xsl:element>
	</xsl:template>

	<xsl:template match="song/title"><!--FIXME-->
	</xsl:template>
	<xsl:template match="song/artist"><!--FIXME-->
	</xsl:template>

	<xsl:template match="sections"><xsl:apply-templates/></xsl:template>

	<xsl:template match="section">
		<xsl:element name="measure">
			<xsl:attribute name="number"><xsl:text>1</xsl:text></xsl:attribute>
		</xsl:element>
	</xsl:template>

	<xsl:template match="fonts">
		<!-- Ignore fonts -->
	</xsl:template>

	<xsl:template match="*">
		<xsl:message>
			<xsl:text>No template matches </xsl:text>
			<xsl:value-of select="name(.)"/>
			<xsl:text>.</xsl:text>
		</xsl:message>
	</xsl:template>
</xsl:stylesheet>
