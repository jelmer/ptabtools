<?xml version='1.0'?>
<!--
	PowerTab to MusicXML converter. 
	(C) 2004 Jelmer Vernooij <jelmer@samba.org>
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:exsl="http://exslt.org/common"
	version="1.1">

	<xsl:output method="xml" encoding="UTF-8" doctype-public="-//Recordare//DTD MusicXML 1.0 Partwise//EN" indent="yes" doctype-system="http://www.musicxml.org/dtds/partwise.dtd"/>

	<xsl:template match="powertab">
		<xsl:element name="score-partwise">
			<xsl:element name="part-list">

			</xsl:element>
			<xsl:apply-templates/>
		</xsl:element>
	</xsl:template>

	<xsl:template match="sections">
		<xsl:apply-templates/>
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

	<xsl:template match="guitar-transcribed-by|bass-transcribed-by"/>

	<xsl:template match="header">
		<xsl:element name="identification">
			<xsl:apply-templates/>
			<xsl:element name="encoding">
				<xsl:for-each select="song/guitar-transcribed-by|song/bass-transcribed-by">
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

	<xsl:template match="song/title"/>
	<xsl:template match="song/artist"/>

	<xsl:template match="section">
		<xsl:element name="part">
		</xsl:element>
	</xsl:template>

</xsl:stylesheet>
